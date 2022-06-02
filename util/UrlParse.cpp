#include "UrlParse.h"
#include "uri-parse.h"
#include "urlcodec.h"
#include <memory>
#include <string.h>
#include <functional>

using namespace DLNetwork;

bool DLNetwork::getUriParams(const char* uri, std::string& streamType, std::string& id, std::map<std::string, std::string>& params, std::string& subPath) {
	char path[256];
	char* p;
	char* sp;
	//struct uri_t* r = uri_parse(uri, strlen(uri));
	std::unique_ptr<struct uri_t, std::function<void(struct uri_t*)>> r(uri_parse(uri, strlen(uri)), [](struct uri_t* p) {uri_free(p); });
	if (!r) {
		return false;
	}
	if (!r->query) {
		return false;
	}

	url_decode(r->path, strlen(r->path), path, sizeof(path));
	if (path[0] == '/') {
		p = path + 1;
	}
	else {
		p = path;
	}
	sp = strchr(p, '/');
	if (!sp) {
		return false;
	}
	*sp = NULL;
	streamType = p; // source type
	p = sp + 1;
	id = p; // source id

	strcpy(path, r->query);
	p = path;
	if (*p == '?') p++;
	sp = strchr(p, '/');
	if (sp) {
		*sp++ = NULL; // remove sub path after params
		subPath = sp;
	}
	do {
		sp = strchr(p, '=');
		if (!sp) {
			break;
		}
		*sp = NULL;
		char* key = p;
		p = sp + 1;
		char* val = p;
		sp = strchr(p, '&');
		if (sp) {
			*sp = NULL;
			p = sp + 1;
		}
		params.emplace(key, val);
	} while (sp);
	return true;
}
