#include <balde.h>
#include "templates/foo.h"

balde_response_t*
hello(balde_app_t *app, balde_request_t *request)
{
    balde_response_t *response = balde_make_response("");
    gchar *name = balde_request_get_arg(request, "name");
    balde_response_set_tmpl_var(response, "name", name != NULL ? name : "World");
    balde_template_foo(response);
    return response;
}


int
main(int argc, char **argv)
{
    balde_app_t *app = balde_app_init();
    balde_app_add_url_rule(app, "hello", "/", BALDE_HTTP_GET, hello);
    balde_app_run(app);
    balde_app_free(app);
}