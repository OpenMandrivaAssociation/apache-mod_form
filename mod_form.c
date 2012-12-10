/********************************************************************
         Copyright (c) 2004-6, WebThing Ltd
         Author: Nick Kew <nick@webthing.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*********************************************************************/

#include <httpd.h>
#include <http_config.h>
#include <http_request.h>
#include <util_filter.h>
#include <apr_strings.h>
#include <http_log.h>
#include <http_protocol.h>

#include "mod_form.h"

module AP_MODULE_DECLARE_DATA form_module ;
static const char* form_std_delim = "&" ;


typedef struct {
  size_t maxsize ;
  int post ;
  int get ;
  const char* delim ;
} form_conf ;
typedef struct {
  apr_table_t* vars ;
  size_t len ;
  int eos ;
  char delim;
} form_ctx ;

static apr_table_t* form_data(request_rec* r) {
  form_ctx* ctx = ap_get_module_config(r->request_config, &form_module) ;
  return ctx ? ctx->vars : NULL ;
}
static const char* form_value(request_rec* r, const char* arg) {
  form_ctx* ctx = ap_get_module_config(r->request_config, &form_module) ;
  if ( ! ctx || ! ctx->vars )
    return NULL ;
  return apr_table_get(ctx->vars, arg) ;
}

static void form_decode(request_rec* r, char* args, const char* delim) {
  form_ctx* ctx = ap_get_module_config(r->request_config, &form_module ) ;
  char* pair ;
  char* last = NULL ;
  char* eq ;
  if ( ! ctx ) {
    ctx = apr_pcalloc(r->pool, sizeof(form_ctx)) ;
    ctx->delim = delim[0];
    ap_set_module_config(r->request_config, &form_module, ctx) ;
  }
  if ( ! ctx->vars ) {
    ctx->vars = apr_table_make(r->pool, 10) ;
  }
  for ( pair = apr_strtok(args, delim, &last) ; pair ;
        pair = apr_strtok(NULL, delim, &last) ) {
    for (eq = pair ; *eq ; ++eq)
      if ( *eq == '+' )
        *eq = ' ' ;
    ap_unescape_url(pair) ;
    eq = strchr(pair, '=') ;
    if ( eq ) {
      *eq++ = 0 ;
      apr_table_merge(ctx->vars, pair, eq) ;
    } else {
      apr_table_merge(ctx->vars, pair, "") ;
    }
  }
}
#define BUFSZ 8192
static apr_status_t form_filter(ap_filter_t *f, apr_bucket_brigade *bbout,
        ap_input_mode_t mode, apr_read_type_e block, apr_off_t nbytes) {
  form_ctx* ctx ;
  char* leftover = NULL ;
  char* eq ;
  char* delim ;
  char* pair ;
  apr_bucket* b ;
  apr_bucket* nextb ;
  apr_bucket_brigade* bb ;
  int rv ;
  int rstat ;
  const char* buf ;
  size_t llen ;
  size_t bytes ;
  size_t readbytes = BUFSZ ;

  if ( ! f->ctx ) {
    f->ctx = ap_get_module_config(f->r->request_config, &form_module ) ;
  }
  ctx = f->ctx ;

  if ( ctx->eos ) {
    APR_BRIGADE_INSERT_TAIL(bbout, apr_bucket_eos_create(bbout->bucket_alloc));
  }

  if ( ! ctx->vars ) {
    ctx->vars = apr_table_make(f->r->pool, 10) ;
  }
  
  bb = apr_brigade_create(f->r->pool, f->r->connection->bucket_alloc) ;
  do {
    rv = ap_get_brigade(f->next, bb, AP_MODE_READBYTES,
        APR_BLOCK_READ, readbytes) ;
    if ( (rv != APR_SUCCESS ) && ( rv != APR_EAGAIN) ) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, f->r, "Reading form data");
      return rv ;
    }
    for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); b = nextb) {
      nextb = APR_BUCKET_NEXT(b) ;
      APR_BUCKET_REMOVE(b) ;
      APR_BRIGADE_INSERT_TAIL(bbout, b) ;
      if ( APR_BUCKET_IS_EOS(b) ) {
        ctx->eos = 1 ;
        /* we still have data in leftover - now it's a complete arg=val */
        if ( leftover ) {
          pair = leftover ;
          for (eq = pair ; *eq ; ++eq)
            if ( *eq == '+' )
              *eq = ' ' ;
          ap_unescape_url(pair) ;
          eq = strchr(pair, '=' ) ;
          if ( eq ) {
            *eq++ = 0 ;
            apr_table_mergen(ctx->vars, pair, eq) ;
          } else {
            apr_table_mergen(ctx->vars, pair, "") ;
          }
        }

      } else {
        if ( ! APR_BUCKET_IS_METADATA(b) ) {
          do {
            bytes = readbytes;
            rstat = apr_bucket_read(b, &buf, &bytes, APR_BLOCK_READ) ;
            if ( rstat == APR_SUCCESS ) {
              ctx->len -= bytes ;
              while ( bytes > 0 ) {
                delim = memchr(buf, ctx->delim, bytes) ;
                if ( delim || (ctx->len == 0) ) {
                  if ( leftover ) {
                    llen = strlen(leftover) ;
                    pair = apr_palloc(f->r->pool, llen + delim - buf + 1) ;
                    memcpy(pair, leftover, llen) ;
                    memcpy(pair+llen, buf, delim-buf) ;
                    pair[llen+delim-buf] = 0 ;
                    leftover = NULL ;
                  } else if ( delim == 0 ) {
                    pair = apr_pmemdup(f->r->pool, buf, bytes + 1) ;
                    pair[bytes] = 0 ;
                  } else {
                    pair = apr_pmemdup(f->r->pool, buf, delim - buf + 1) ;
                    pair[delim-buf] = 0 ;
                  }
                  for (eq = pair ; *eq ; ++eq)
                    if ( *eq == '+' )
                      *eq = ' ' ;
                  ap_unescape_url(pair) ;
                  eq = strchr(pair, '=' ) ;
                  if ( eq ) {
                    *eq++ = 0 ;
                    apr_table_mergen(ctx->vars, pair, eq) ;
                  } else {
                    apr_table_mergen(ctx->vars, pair, "") ;
                  }
                } else {
                  leftover = apr_pstrndup(f->r->pool, buf, bytes) ;
                }
                if ( delim++ ) {
                  bytes -= (delim - buf ) ;
                  buf = delim ;
                } else {
                  bytes = 0 ;   /* rest of buf is now in leftover - ignore */
                }
              }
            }
          } while ( rstat == APR_EAGAIN ) ;
          if ( rstat != APR_SUCCESS ) {
            return rstat ;
          }
        }
      }
    }
    apr_brigade_cleanup(bb) ;
  } while ( ! ctx->eos ) ;
  apr_brigade_destroy(bb) ;
  return APR_SUCCESS ;
}
static int form_fixups(request_rec* r) {
  form_conf* conf ;
  form_ctx* ctx = NULL ;
  const char* arg ;
  switch ( r->method_number ) {
   case M_GET:
    conf = ap_get_module_config(r->per_dir_config, &form_module) ;
    if ( conf->get != 1 ) {
      return DECLINED ;
    }
    if ( r->args ) {
      if ( strlen(r->args) > conf->maxsize ) {
        return HTTP_REQUEST_URI_TOO_LARGE ;
      }
      form_decode(r, r->args, conf->delim) ;
    }
    return OK ;
   case M_POST:
    conf = ap_get_module_config(r->per_dir_config, &form_module) ;
    if ( conf->post <= 0 ) {
      return DECLINED ;
    }
    arg = apr_table_get(r->headers_in, "Content-Type") ;
    if ( ! arg || strcasecmp(arg, "application/x-www-form-urlencoded" ) ) {
      return DECLINED ;
    }
    arg = apr_table_get(r->headers_in, "Content-Length") ;
    if ( arg ) {
      ctx = apr_pcalloc(r->pool, sizeof(form_ctx)) ;
      ctx->len = atoi(arg) ;
      if ( ctx->len > conf->maxsize ) {
        return HTTP_REQUEST_ENTITY_TOO_LARGE ;
      }
    }
    ap_add_input_filter("form-vars", NULL, r, r->connection) ;
    break ;
   default:
    return DECLINED ;
  }
  if ( ! ctx ) {
    ctx = apr_pcalloc(r->pool, sizeof(form_ctx)) ;
  }
  ctx->delim = conf->delim[0];
  ap_set_module_config(r->request_config, &form_module, ctx) ;
  return OK ;
}

static void form_hooks(apr_pool_t* pool) {
  ap_hook_fixups(form_fixups, NULL, NULL, APR_HOOK_MIDDLE) ;
  ap_register_input_filter("form-vars", form_filter, NULL, AP_FTYPE_RESOURCE) ;
  APR_REGISTER_OPTIONAL_FN(form_data) ;
  APR_REGISTER_OPTIONAL_FN(form_value) ;
}
static const command_rec form_cmds[] = {
  AP_INIT_TAKE1("FormMaxSize", ap_set_int_slot,
        (void*)APR_OFFSETOF(form_conf, maxsize),
        OR_OPTIONS, "Max size") ,
  AP_INIT_FLAG("FormGET", ap_set_flag_slot,
        (void*)APR_OFFSETOF(form_conf, get),
        OR_OPTIONS, "Decode GET args (query string)") ,
  AP_INIT_FLAG("FormPOST", ap_set_flag_slot,
        (void*)APR_OFFSETOF(form_conf, post),
        OR_OPTIONS, "Decode POST args") ,
  AP_INIT_TAKE1("FormDelim", ap_set_string_slot,
        (void*)APR_OFFSETOF(form_conf, delim),
        OR_OPTIONS, "Form args delimiter(s)") ,
  {NULL}
} ;
static void* form_cr_conf(apr_pool_t* pool, char* x) {
  form_conf* conf = apr_palloc(pool, sizeof(form_conf)) ;
  conf->maxsize = (size_t)-1 ;
  conf->get = conf->post = -1 ;
  conf->delim = form_std_delim ;
  return conf ;
}
static void* form_merge_conf(apr_pool_t* pool, void* BASE, void* ADD) {
  form_conf* base = BASE ;
  form_conf* add = ADD ;
  form_conf* conf = apr_palloc(pool, sizeof(form_conf)) ;
  conf->maxsize = (add->maxsize != (size_t)-1) ? add->maxsize : base->maxsize;
  conf->get = ( add->get != -1 ) ? add->get : base->get ;
  conf->post = ( add->post != -1 ) ? add->post : base->post ;
  conf->delim = ( add->delim != form_std_delim ) ? add->delim : base->delim ;
  return conf ;
}
module AP_MODULE_DECLARE_DATA form_module = {
        STANDARD20_MODULE_STUFF,
        form_cr_conf,
        form_merge_conf,
        NULL,
        NULL,
        form_cmds,
        form_hooks
} ;
