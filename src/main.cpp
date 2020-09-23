#include <httplib.h>
#include <filesystem>
#include <fmt/core.h>

//hoedown
#include "document.h"
#include "html.h"

using namespace std;
using namespace filesystem;

#define PAGES_DIR	"pages"
#define POSTS_DIR	"posts"

enum renderer_type {
	RENDERER_HTML,
	RENDERER_HTML_TOC
};

struct extension_category_info {
	unsigned int flags;
	const char *option_name;
	const char *label;
};

struct extension_info {
	unsigned int flag;
	const char *option_name;
	const char *description;
};

struct html_flag_info {
	unsigned int flag;
	const char *option_name;
	const char *description;
};

#define DEF_IUNIT 1024
#define DEF_OUNIT 64
#define DEF_MAX_NESTING 16

struct option_data {
	const char *basename;
	int done;

	/* time reporting */
	int show_time;

	/* I/O */
	size_t iunit;
	size_t ounit;
	const char *filename;

	/* renderer */
	enum renderer_type renderer;
	int toc_level;
	hoedown_html_flags html_flags;

	/* parsing */
	hoedown_extensions extensions;
	size_t max_nesting;
};


string render_md_file(const path& p){
	struct option_data data;
	FILE *file = stdin;
	hoedown_buffer *ib, *ob;
	hoedown_renderer *renderer = NULL;
	void (*renderer_free)(hoedown_renderer *) = NULL;
	hoedown_document *document;

	/* Parse options */
	data.basename = NULL;
	data.done = 0;
	data.show_time = 0;
	data.iunit = DEF_IUNIT;
	data.ounit = DEF_OUNIT;
	data.filename = p.string().c_str();
	data.renderer = RENDERER_HTML;
	data.toc_level = 0;
	data.html_flags = hoedown_html_flags(0);
	data.extensions = HOEDOWN_EXT_FENCED_CODE;
	data.max_nesting = DEF_MAX_NESTING;

	fprintf(stderr, "hello dawg, parsing %s\n", p.string().c_str());
	file = fopen(p.string().c_str(), "r");
	if (!file) {
		fprintf(stderr, "Unable to open input file \"%s\": %s\n", data.filename, strerror(errno));
		return "";
	}

	/* Read everything */
	ib = hoedown_buffer_new(data.iunit);

	if (hoedown_buffer_putf(ib, file)) {
		fprintf(stderr, "I/O errors found while reading input.\n");
		return "";
	}

	if (file != stdin) fclose(file);

	/* Create the renderer */
	switch (data.renderer) {
		case RENDERER_HTML:
			renderer = hoedown_html_renderer_new(data.html_flags, data.toc_level);
			renderer_free = hoedown_html_renderer_free;
			break;
		case RENDERER_HTML_TOC:
			renderer = hoedown_html_toc_renderer_new(data.toc_level);
			renderer_free = hoedown_html_renderer_free;
			break;
	};

	/* Perform Markdown rendering */
	ob = hoedown_buffer_new(data.ounit);
	document = hoedown_document_new(renderer, data.extensions, data.max_nesting);

	hoedown_document_render(document, ob, ib->data, ib->size);

	/* Cleanup */
	hoedown_buffer_free(ib);
	hoedown_document_free(document);
	renderer_free(renderer);

	/* Write the result to stdout */
	// (void)fwrite(ob->data, 1, ob->size, stdout);

	string contentString((const char*)ob->data, ob->size);
	hoedown_buffer_free(ob);
	return contentString;
}

string get_pages_list(){
	stringstream ss;
	ss << "<h4>pages</h4><ul>";
	for(auto& p: filesystem::directory_iterator(PAGES_DIR)){
		if(!p.is_directory()){
	    	ss <<  fmt::format("<li><a href='/pages/{0}'>{0}</a></li>", p.path().filename().string());
		}
	}
	ss << "</ul>" << endl;
	return ss.str();
}

string get_posts_list(){
	stringstream ss;
	ss << "<h4>posts</h4><ul>";
	for(auto& p: filesystem::directory_iterator(POSTS_DIR)){
		if(!p.is_directory()){
			ss <<  fmt::format("<li><a href='/posts/{0}'>{0}</a></li>", p.path().filename().string());
		}
	}
	ss << "</ul>" << endl;
	return ss.str();
}

std::string get_file_contents(const char *filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if(in){
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

int main(void)
{
	using namespace httplib;

	Server svr;

	svr.Get("/", [](const Request& req, Response& res) {
		res.set_content(get_pages_list() + get_posts_list(), "text/html");
	});

	svr.Get(R"(/pages/([a-zA-Z0-9_\-\.]+))", [&](const Request& req, Response& res) {
		auto pagePath = req.matches[1];
		res.set_content(render_md_file(string("./pages/") + pagePath.str()), "text/html");
	});

	svr.Get(R"(/posts/([a-zA-Z0-9_\-\.]+))", [&](const Request& req, Response& res) {
		auto postPath = req.matches[1];
		res.set_content(render_md_file(string("./posts/") + postPath.str()), "text/html");
	});

	svr.set_error_handler([](const Request& req, Response& res) {
		fmt::print("error\n");
	});

	svr.Get("/stop", [&](const Request& req, Response& res) {
		svr.stop();
	});

	svr.Get("/favicon.ico", [&](const Request& req, Response& res) {
		if(filesystem::exists("./favicon.ico")){
			res.set_content(get_file_contents("./favicon.ico"), "image/webp");
			return res.status = 200;
		} else {
			return res.status = 404;
		}
	});

	svr.Get(R"(/static/([a-zA-Z0-9_\-\.]+))", [&](const Request& req, Response& res) {
		string target_path = string("./static/") + req.matches[1].str();
		fmt::print("requesting static uri {}\n", target_path);
		if(filesystem::exists(target_path)){
			res.set_content(get_file_contents(target_path.c_str()), "");
			return res.status = 200;
		} else {
			return res.status = 404;
		}
	});

	const char* ip = "0.0.0.0";
	unsigned int port = 1993;
	fmt::print("starting to listen on {}:{}\n", ip, port);
	svr.listen(ip, port);
}
