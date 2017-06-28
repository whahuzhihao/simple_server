/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_SIMPLE_SERVER_H
#define PHP_SIMPLE_SERVER_H
#include "common.h"

#define PHP_SIMPLE_SERVER_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_SIMPLE_SERVER_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SIMPLE_SERVER_API __attribute__ ((visibility("default")))
#else
#	define PHP_SIMPLE_SERVER_API
#endif


extern zend_module_entry simple_server_module_entry;
#define phpext_simple_server_ptr &simple_server_module_entry

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(simple_server)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(simple_server)
*/

/* In every utility function you add that needs to use variables 
   in php_simple_server_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as SIMPLE_SERVER_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define SIMPLE_SERVER_G(v) TSRMG(simple_server_globals_id, zend_simple_server_globals *, v)
#else
#define SIMPLE_SERVER_G(v) (simple_server_globals.v)
#endif



#include "src/include/tcp_event_server.h"
#include <map>
using std::map;
extern map<int, TcpEventServer *> server_map;

static inline void* server_get_object(zval *object)
{
#if PHP_MAJOR_VERSION < 7
    zend_object_handle handle = Z_OBJ_HANDLE_P(object);
#else
    int handle = (int)Z_OBJ_HANDLE(*object);
#endif
    assert(handle < server_map.size);
    return server_map[(int)handle];
}

static inline void server_set_object(zval *object, void *ptr)
{
#if PHP_MAJOR_VERSION < 7
    zend_object_handle handle = Z_OBJ_HANDLE_P(object);
#else
    int handle = (int) Z_OBJ_HANDLE(*object);
#endif
    server_map[(int)handle] = (TcpEventServer*)ptr;
    return ;
}

static inline void server_del_object(zval *object){
#if PHP_MAJOR_VERSION < 7
    zend_object_handle handle = Z_OBJ_HANDLE_P(object);
#else
    int handle = (int) Z_OBJ_HANDLE(*object);
#endif
    server_map.erase((int)handle);
    return ;
}

static inline int check_callable(zval *callback TSRMLS_CC)
{
    if (!callback || ZVAL_IS_NULL(callback))
    {
        return 0;
    }

    char *func_name = NULL;
    int ret = zend_is_callable(callback, 0, &func_name TSRMLS_CC)? 1:0;

    if (func_name)
    {
        efree(func_name);
        func_name = NULL;
    }

    return ret;
}

//simple_server
PHP_METHOD(simple_server, __construct);
PHP_METHOD(simple_server, __destruct);
PHP_METHOD(simple_server, start);
PHP_METHOD(simple_server, on);
PHP_METHOD(simple_server, send);

static void simple_server_onreceive(ServerCbParam *param);
static void simple_server_onconnect(ServerCbParam *param);
static void simple_server_onclose(ServerCbParam *param);

//----//

#endif	/* PHP_SIMPLE_SERVER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
