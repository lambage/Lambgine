#include <stdafx.h>
#include <Lambgine.h>
#include "LambLua.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lua_c

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#define EXIT_CONDITION -2

#if !defined(LUA_PROMPT)
#define LUA_PROMPT		"> "
#define LUA_PROMPT2		">> "
#endif

#define LUA_PROGNAME		"lua"

#define LUA_MAXINPUT		512

#include <io.h>
#include <stdio.h>
#define lua_stdin_is_tty()	_isatty(_fileno(stdin))

#define lua_readline(L,b,p) \
	((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	fgets(b, LUA_MAXINPUT, stdin) != NULL)  /* get line */
#define lua_saveline(L,idx)	{ (void)L; (void)idx; }
#define lua_freeline(L,b)	{ (void)L; (void)b; }

#include <stdio.h>
#define luai_writestring(s,l)	fwrite((s), sizeof(char), (l), stdout)
#define luai_writeline()	(luai_writestring("\n", 1), fflush(stdout))

static lua_State *globalL = NULL;

static const char *progname = LUA_PROGNAME;

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

struct LambLua::LambLuaImpl
{
	lua_State* L = nullptr;
	LambLuaImpl()
	{
		L = luaL_newstate();
		luaL_openlibs(L);
	}

	~LambLuaImpl()
	{
		if (L != nullptr)
		{
			lua_close(L);
		}
	}

	static void lstop(lua_State *L, lua_Debug *ar) {
		(void)ar;  /* unused arg. */
		lua_sethook(L, NULL, 0, 0);
		luaL_error(L, "interrupted!");
	}


	static void laction(int i) {
		signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
							terminate process (default action) */
		lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
	}

	static void l_message(const char *pname, const char *msg) {
		if (pname) luai_writestringerror("%s: ", pname);
		luai_writestringerror("%s\n", msg);
	}


	static int report(lua_State *L, int status) {
		if (status != LUA_OK && !lua_isnil(L, -1)) {
			const char *msg = lua_tostring(L, -1);
			if (msg == NULL) msg = "(error object is not a string)";
			l_message(progname, msg);
			lua_pop(L, 1);
			/* force a complete garbage collection in case of errors */
			lua_gc(L, LUA_GCCOLLECT, 0);
		}
		return status;
	}


	/* the next function is called unprotected, so it must avoid errors */
	static void finalreport(lua_State *L, int status) {
		if (status != LUA_OK) {
			const char *msg = (lua_type(L, -1) == LUA_TSTRING) ? lua_tostring(L, -1)
				: NULL;
			if (msg == NULL) msg = "(error object is not a string)";
			l_message(progname, msg);
			lua_pop(L, 1);
		}
	}


	static int traceback(lua_State *L) {
		const char *msg = lua_tostring(L, 1);
		if (msg)
			luaL_traceback(L, L, msg, 1);
		else if (!lua_isnoneornil(L, 1)) {  /* is there an error object? */
			if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
				lua_pushliteral(L, "(no error message)");
		}
		return 1;
	}


	static int docall(lua_State *L, int narg, int nres) {
		int status;
		int base = lua_gettop(L) - narg;  /* function index */
		lua_pushcfunction(L, traceback);  /* push traceback function */
		lua_insert(L, base);  /* put it under chunk and args */
		globalL = L;  /* to be available to 'laction' */
		signal(SIGINT, laction);
		status = lua_pcall(L, narg, nres, base);
		signal(SIGINT, SIG_DFL);
		lua_remove(L, base);  /* remove traceback function */
		return status;
	}

	static const char *get_prompt(lua_State *L, int firstline) {
		const char *p;
		lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2");
		p = lua_tostring(L, -1);
		if (p == NULL) p = (firstline ? LUA_PROMPT : LUA_PROMPT2);
		return p;
	}



	static int incomplete(lua_State *L, int status) {
		if (status == LUA_ERRSYNTAX) {
			size_t lmsg;
			const char *msg = lua_tolstring(L, -1, &lmsg);
			if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
				lua_pop(L, 1);
				return 1;
			}
		}
		return 0;  /* else... */
	}

	static int pushline(lua_State *L, int firstline) {
		char buffer[LUA_MAXINPUT];
		char *b = buffer;
		size_t l;
		const char *prmt = get_prompt(L, firstline);
		int readstatus = lua_readline(L, b, prmt);
		lua_pop(L, 1);  /* remove result from 'get_prompt' */
		if (readstatus == 0)
			return 0;  /* no input */
		l = strlen(b);
		if (l > 0 && b[l - 1] == '\n')  /* line ends with newline? */
			b[l - 1] = '\0';  /* remove it */
		if (firstline && b[0] == '=')  /* first line starts with `=' ? */
			lua_pushfstring(L, "return %s", b + 1);  /* change it to `return' */
		else
			lua_pushstring(L, b);
		lua_freeline(L, b);
		return 1;
	}

	static int loadline(lua_State *L) {
		int status;
		lua_settop(L, 0);
		if (!pushline(L, 1))
			return -1;  /* no input */
		for (;;) {  /* repeat until gets a complete line */
			size_t l;
			const char *line = lua_tolstring(L, 1, &l);
			if (strcmp(line, "q") == 0 || strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "qut") == 0)
			{
				return EXIT_CONDITION;
			}
			status = luaL_loadbuffer(L, line, l, "=stdin");
			if (!incomplete(L, status)) break;  /* cannot try to add lines? */
			if (!pushline(L, 0))  /* no more input? */
				return -1;
			lua_pushliteral(L, "\n");  /* add a new line... */
			lua_insert(L, -2);  /* ...between the two lines */
			lua_concat(L, 3);  /* join them */
		}
		lua_saveline(L, 1);
		lua_remove(L, 1);  /* remove line */
		return status;
	}

	int DoTerminal()
	{
		int status;
		const char *oldprogname = progname;
		progname = NULL;
		while ((status = loadline(L)) != -1) {
			if (status == EXIT_CONDITION)
			{
				lua_settop(L, 0);  /* clear stack */
				luai_writeline();
				progname = oldprogname;
				return LUA_OK;
			}
			if (status == LUA_OK) status = docall(L, 0, LUA_MULTRET);
			report(L, status);
			if (status == LUA_OK && lua_gettop(L) > 0) {  /* any result to print? */
				luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
				lua_getglobal(L, "print");
				lua_insert(L, 1);
				if (lua_pcall(L, lua_gettop(L) - 1, 0, 0) != LUA_OK)
					l_message(progname, lua_pushfstring(L,
					"error calling " LUA_QL("print") " (%s)",
					lua_tostring(L, -1)));
			}
		}
		lua_settop(L, 0);  /* clear stack */
		luai_writeline();
		progname = oldprogname;
		return status;
	}
};

LAMBGINE_API LambLua::LambLua() :
mImpl(std::make_unique<LambLua::LambLuaImpl>())
{
}


LAMBGINE_API LambLua::~LambLua()
{
}

LAMBGINE_API lua_State* LambLua::GetLuaState()
{
	return mImpl->L;
}

LAMBGINE_API int LambLua::DoTerminal()
{
	return mImpl->DoTerminal();
}