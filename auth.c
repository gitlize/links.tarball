#include "links.h"

struct list_head auth = {&auth, &auth};

struct http_auth {
	struct http_auth *next;
	struct http_auth *prev;
	char *host;
	int port;
	char *realm;
	char *user;
	char *password;
	char *directory;
	char *user_password_encoded;
	int proxy;
};

unsigned char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *base64_encode(unsigned char *in)
{
	unsigned char *out, *outstr;
	int inlen = strlen(in);
	if (!(outstr = out = mem_alloc(((inlen / 3) + 1) * 4 + 1 ))) return NULL;
	while (inlen >= 3) {
		*out++ = base64_chars[ (int)(*in >> 2) ];
		*out++ = base64_chars[ (int)((*in << 4 | *(in + 1) >> 4) & 63) ];
		*out++ = base64_chars[ (int)((*(in + 1) << 2 | *(in + 2) >> 6) & 63) ];
		*out++ = base64_chars[ (int)(*(in + 2) & 63) ];
		inlen -= 3; in += 3;
	}
	if (inlen == 1) {
		*out++ = base64_chars[ (int)(*in >> 2) ];
		*out++ = base64_chars[ (int)(*in << 4 & 63) ];
		*out++ = '=';
		*out++ = '=';
	}
	if (inlen == 2) {
		*out++ = base64_chars[ (int)(*in >> 2) ];
		*out++ = base64_chars[ (int)((*in << 4 | *(in + 1) >> 4) & 63) ];
		*out++ = base64_chars[ (int)((*(in + 1) << 2) & 63) ];
		*out++ = '=';
	}
	*out = 0;
	return outstr;
}

unsigned char *get_auth_realm(unsigned char *url, unsigned char *head, int proxy)
{
	unsigned char *h = parse_http_header(head, !proxy ? "WWW-Authenticate" : "Proxy-Authenticate", NULL);
	unsigned char *q;
	unsigned char *r;
	int l;
	if (!h) {
		xxx:
		if (proxy) return stracpy(http_proxy);
		h = get_host_name(url);
		if (h) return h;
		return stracpy("");
	}
	q = strchr(h, '"');
	if (!q) goto xxx;
	q++;
	r = init_str();
	l = 0;
	while (*q && *q != '"') {
		if (*q == '\\' && !*++q) break;
		add_chr_to_str(&r, &l, *q++);
	}
	mem_free(h);
	return r;
}

char *get_auth_string(char *url)
{
	struct http_auth *a;
	unsigned char *host;
	int port;
	unsigned char *r = NULL;
	int l = 0;
	if (!(host = get_host_name(url))) return NULL;
	port = get_port(url);
	if (upcase(url[0]) == 'P') {
		foreach(a, auth) {
			if (a->proxy && !strcasecmp(a->host, host) && a->port == port) {
				if (!r) r = init_str();
				add_to_str(&r, &l, "Proxy-Authorization: Basic ");
				add_to_str(&r, &l, a->user_password_encoded);
				add_to_str(&r, &l, "\r\n");
				break;
			}
		}
		url = get_url_data(url);
		mem_free(host);
		if (!(host = get_host_name(url))) return NULL;
		port = get_port(url);
	}
	foreach(a, auth) if (!a->proxy && !strcasecmp(a->host, host) && a->port == port) {
		unsigned char *d, *data;
		data = get_url_data(url);
		d = strrchr(data, '/');
		if (!d) d = data;
		else d++;
		if (d - data >= strlen(a->directory) && !memcmp(data, a->directory, strlen(a->directory))) {
			if (!r) r = init_str();
			add_to_str(&r, &l, "Authorization: Basic ");
			add_to_str(&r, &l, a->user_password_encoded);
			add_to_str(&r, &l, "\r\n");
			break;
		}
	}
	mem_free(host);
	return r;
}

void free_auth_entry(struct http_auth *a)
{
	mem_free(a->host);
	mem_free(a->realm);
	mem_free(a->user);
	mem_free(a->password);
	if (a->directory) mem_free(a->directory);
	mem_free(a->user_password_encoded);
	del_from_list(a);
	mem_free(a);
}

void cleanup_auth()
{
	while (!list_empty(auth)) free_auth_entry(auth.next);
}

void add_auth(unsigned char *url, unsigned char *realm, unsigned char *user, unsigned char *password, int proxy)
{
	struct http_auth *a;
	unsigned char *host;
	unsigned char *p;
	int port;
	if (!proxy) {
		host = get_host_name(url);
		port = get_port(url);
	} else {
		char *p = get_proxy(url);
		host = get_host_name(p);
		port = get_port(p);
		mem_free(p);
	}
	if (!host) return;
	foreach(a, auth) if (a->proxy == proxy && !strcasecmp(a->host, host) && a->port == port && !strcmp(a->realm, realm)) {
		a = a->prev;
		free_auth_entry(a->next);
	}
	a = mem_alloc(sizeof(struct http_auth));
	a->host = host;
	a->port = port;
	a->realm = stracpy(realm);
	a->user = stracpy(user);
	a->password = stracpy(password);
	if (!proxy) {
		unsigned char *data = stracpy(get_url_data(url));
		unsigned char *d = strrchr(data, '/');
		if (d) d[1] = 0;
		else data[0] = 0;
		a->directory = data;
	} else a->directory = NULL;
	a->proxy = proxy;
	p = mem_alloc(strlen(a->user) + strlen(a->password) + 2);
	strcpy(p, a->user);
	strcat(p, ":");
	strcat(p, a->password);
	a->user_password_encoded = base64_encode(p);
	mem_free(p);
	add_to_list(auth, a);
}

int find_auth(unsigned char *url, unsigned char *realm)
{
	struct http_auth *a;
	unsigned char *data, *d;
	unsigned char *host = get_host_name(url);
	int port = get_port(url);
	if (!host) return -1;
	data = stracpy(get_url_data(url));
	d = strrchr(data, '/');
	if (d) d[1] = 0;
	foreach(a, auth) if (!a->proxy && !strcasecmp(a->host, host) && a->port == port && !strcmp(a->realm, realm) && strcmp(a->directory, data)) {
		mem_free(a->directory);
		a->directory = data;
		mem_free(host);
		del_from_list(a);
		add_to_list(auth, a);
		return 0;
	}
	mem_free(host);
	mem_free(data);
	return -1;
}

