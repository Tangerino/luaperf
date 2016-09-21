/* 
 * File:   main.c
 * Author: Carlos Tangerinos
 *
 * Created on June 15, 2016, 6:55 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <unistd.h>

const char *formula =
        "ww = w * w;"
        "vv = v * v;"
        "s = ww + vv;"
        "r = math.sqrt(s);"
        "pf = 1 / r;"
        "return pf;";

static int timeval_subtract(struct timeval *result, struct timeval *startTime, struct timeval *endTime) {
    /* Perform the carry for the later subtraction by updating y. */
    if (endTime->tv_usec < startTime->tv_usec) {
        int nsec = (startTime->tv_usec - endTime->tv_usec) / 1000000 + 1;
        startTime->tv_usec -= 1000000 * nsec;
        startTime->tv_sec += nsec;
    }
    if (endTime->tv_usec - startTime->tv_usec > 1000000) {
        int nsec = (endTime->tv_usec - startTime->tv_usec) / 1000000;
        startTime->tv_usec += 1000000 * nsec;
        startTime->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = endTime->tv_sec - startTime->tv_sec;
    result->tv_usec = endTime->tv_usec - startTime->tv_usec;

    /* Return 1 if result is negative. */
    return endTime->tv_sec < startTime->tv_sec;
}

void timerStart(struct timeval *ti) {
    gettimeofday(ti, NULL);
}

clock_t timerLap(struct timeval *ti) {
    struct timeval lap;
    struct timeval dif;
    gettimeofday(&lap, NULL);
    timeval_subtract(&dif, ti, &lap);
    return (clock_t) (dif.tv_sec * 1000 + (dif.tv_usec / 1000));
}

static char *tt2iso8602Utc(time_t tt, char *dt, size_t maxLen) {
    struct tm loctime;
    gmtime_r(&tt, &loctime);
    strftime(dt, maxLen, "%Y-%m-%d %H:%M:%S", &loctime);
    return dt;
}

char *callFormula(lua_State *L, const char *code, double *result) {
    char *returnMessage = NULL;
    if ((luaL_loadstring(L, code) || lua_pcall(L, 0, 1, 0))) {
        returnMessage = strdup((char *) lua_tostring(L, -1));
    } else {
        if (lua_isboolean(L, -1)) {
            *result = (double) lua_toboolean(L, -1);
            if (*result) *result = 1;
        } else if (lua_isnumber(L, -1)) {
            *result = (double) lua_tonumber(L, -1);
        } else {
            /*
                #define LUA_TNIL		0
                #define LUA_TBOOLEAN		1
                #define LUA_TLIGHTUSERDATA	2
                #define LUA_TNUMBER		3
                #define LUA_TSTRING		4
                #define LUA_TTABLE		5
                #define LUA_TFUNCTION		6
                #define LUA_TUSERDATA		7
                #define LUA_TTHREAD		8
             */
            printf("Bad return value. Type %d", lua_type(L, -1));
            returnMessage = strdup("Formula must return a number as in: return a + b");
        }
        lua_pop(L, 1);
    }
    return returnMessage;
}

/*
 * 
 */
int main(int argc, char** argv) {
    lua_State *L = luaL_newstate();
    if (L) {
        luaL_openlibs(L);
        int i;
        struct timeval timer;
        timerStart(&timer);
        for (i = 0; i < 10000000; i++) {
            time_t tt = time(NULL);
            char dt[64];
            tt2iso8602Utc(tt, dt, sizeof (dt));
            lua_pushstring(L, dt);
            lua_setglobal(L, "ts"); // push the time stamp
            lua_pushnumber(L, i);
            lua_setglobal(L, "w"); // active power        
            lua_pushnumber(L, i);
            lua_setglobal(L, "v"); // reactive power 
            double result = 0;
            char *error = callFormula(L, formula, &result);
            if (error) {
                printf("Lua error: %s", error);
                free(error);
                break;
            }
        }
        clock_t elapsed = timerLap(&timer);
        if (elapsed > 0) {
            int cps = (i / elapsed) * 60000;
            printf("Execution of %d functions took %d milli seconds. %d calculations per second", (int) i, (int) elapsed, cps);
        } else {
            printf("It was too fast....");
        }
        lua_close(L);
    }
    return (EXIT_SUCCESS);
}

