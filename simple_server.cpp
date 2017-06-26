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
extern "C" {
    #ifdef HAVE_CONFIG_H
    #include "config.h"
    #endif
}
#include "php_simple_server.h"
//全局的对象map
map<int, TcpEventServer *> server_map;

/* If you declare any globals in php_simple_server.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(simple_server)
*/

/* True global resources - no need for thread safety here */
static int le_simple_server;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("simple_server.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_simple_server_globals, simple_server_globals)
    STD_PHP_INI_ENTRY("simple_server.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_simple_server_globals, simple_server_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_simple_server_compiled(string arg)
   Return a string to confirm that the module is compiled in */
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_simple_server_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_simple_server_init_globals(zend_simple_server_globals *simple_server_globals)
{
	simple_server_globals->global_value = 0;
	simple_server_globals->global_string = NULL;
}
*/
/* }}} */
extern void simple_server_init(int module_number TSRMLS_DC);
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(simple_server)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
    simple_server_init(module_number TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(simple_server)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(simple_server)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(simple_server)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(simple_server)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "simple_server support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ simple_server_functions[]
 *
 * Every user visible function must have an entry in simple_server_functions[].
 */
const zend_function_entry simple_server_functions[] = {
	PHP_FE_END	/* Must be the last line in simple_server_functions[] */
};
/* }}} */

/* {{{ simple_server_module_entry
 */
zend_module_entry simple_server_module_entry = {
	STANDARD_MODULE_HEADER,
	"simple_server",
	simple_server_functions,
	PHP_MINIT(simple_server),
	PHP_MSHUTDOWN(simple_server),
	PHP_RINIT(simple_server),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(simple_server),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(simple_server),
	PHP_SIMPLE_SERVER_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

extern "C"
{
    #ifdef COMPILE_DL_SIMPLE_SERVER
    ZEND_GET_MODULE(simple_server)
    #endif
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
