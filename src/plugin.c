/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2014 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 20

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 2512
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

#define PLUGIN_NAME "Extended Info"
#define PLUGIN_AUTHOR "Bluscream"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_CONTACT "admin@timo.de.vc"

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"Extended Info";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Extended Info";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Extended Info";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Bluscream";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Shows you more informations.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "Extended Info";
}

/*
* Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
* function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
* Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
* "data" to NULL to have the client ignore the info data.
*/
//int requested;
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	int cl_type, sid, talkpower, sqnvp, ownid, unread, diff;
	uint64 cgid, mmbu, mmbd, tmbu, tmbd, created, iconid, cfs, cfp;
	char buf[80], buf2[80], buf4[80];//, time_1[80], time_2[80], time_3[80], time_4[80], time_5[80], time_6[80], time_7[80];
	char* buf3;
	char* plt;
	char* plk;
	char* plc;
	char* pls;
	char* ping;
	char* clienttype;
	char* string;
	char* mid;
	char* sgids;
	char* gip;
	char* ip;
	//char* cln;
	//char* clp;
	//char* teststr;
	char* clc;
	char* clb;
	char *queries;
	time_t now;
	struct tm  ts;

	switch (type) {
	case PLUGIN_SERVER:
		//requested = 1;
		//ts3Functions.requestServerVariables(serverConnectionHandlerID);
		if (ts3Functions.getServerVariableAsInt(serverConnectionHandlerID, VIRTUALSERVER_ID, &sid) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_MACHINE_ID, &mid) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_UNIQUE_IDENTIFIER, &string) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_IP, &gip) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientID(serverConnectionHandlerID, &ownid) == ERROR_ok) {
			if (ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, ownid, 6, &ip) != ERROR_ok) {
				return;
			}
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_CREATED, &created) != ERROR_ok) {
			return;
		}
		else {
			now = time(NULL);
			//tim = *(localtime(&now));
			diff = difftime(now, created);
			//tim = localtime(&created);
			//i = strftime(time_now, 30, "%b %d, %Y; %H:%M:%S\n", &tim);
			//strftime(created, sizeof(created), "%a %Y-%m-%d %H:%M:%S %Z", &created);
			//created2 = created / 86400;

			// Get current time
			time(&now);
			// Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
			ts = *localtime(&now);
			strftime(buf, sizeof(buf), "%A, %d.%m.%Y %H:%M:%S", &ts);
			//strftime(buf2, sizeof(buf2), "%a %d.%m.%Y %H:%M:%S", &tim);
			ctime_s(buf2, sizeof(buf2), &created);
			buf3 = strtok(buf2, "\n");
			//sscanf(buf3, "%s %s %s %s:%s:%s %s", time_1, time_2, time_3, time_4, time_5, time_6, time_7);
			//sprintf(buf4, "%s %s %s %s %s:%s:%s", time_1, time_3, time_2, time_7, time_4, time_5, time_6);
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_MONTH_BYTES_UPLOADED, &mmbu) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_MONTH_BYTES_DOWNLOADED, &mmbd) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_BYTES_UPLOADED, &tmbu) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_BYTES_DOWNLOADED, &tmbd) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_TOTAL, &plt) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_KEEPALIVE, &plk) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_CONTROL, &plc) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_SPEECH, &pls) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PING, &ping) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_ICON_ID, &iconid) != ERROR_ok) {
			return;
		}
		/*if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_ANTIFLOOD_POINTS_TICK_REDUCE, &pls) != ERROR_ok) {
			return;
		} 
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_ANTIFLOOD_POINTS_NEEDED_COMMAND_BLOCK, &pls) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_ANTIFLOOD_POINTS_NEEDED_IP_BLOCK, &pls) != ERROR_ok) {
			return;
		}*/
		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "%s\nPing: [color=darkorange]%s[/color] ms\nIcon ID: [color=darkgrey]%u[/color]\nGiven IP: [color=gray]%s[/color]\nResolved IP: [color=green]%s[/color]\nMachine ID: [color=blue]%s[/color]\nVirtualserver ID: [color=blue]%i[/color]\nUID: [color=blue]%s[/color]\nCreated: %s (%u)\nMonthly Traffic: Up: [color=blue]%u[/color] B | Down: [color=red]%u[/color] B\nTotal Traffic: Up: [color=darkblue]%u[/color] B | Down: [color=firebrick]%u[/color] B\nLoss Total: [color=magenta]%s[/color]%% | Keepalive: [color=magenta]%s[/color]%% | Control: [color=magenta]%s[/color]%% | Speech: [color=magenta]%s[/color]%%", buf, ping, iconid, gip, ip, mid, sid, string, buf3, diff, mmbu, mmbd, tmbu, tmbd, plt, plk, plc, pls);
		ts3Functions.freeMemory(string);
		ts3Functions.freeMemory(mid);
		ts3Functions.freeMemory(ip);
		ts3Functions.freeMemory(gip);
		break;
	case PLUGIN_CHANNEL:
		now = time(NULL);
		time(&now);
		ts = *localtime(&now);
		strftime(buf, sizeof(buf), "%A, %d.%m.%Y %H:%M:%S", &ts);
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, (anyID)id, CHANNEL_NAME_PHONETIC, &string) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CHANNEL_ICON_ID, &iconid) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CHANNEL_FORCED_SILENCE, &cfs) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CHANNEL_FLAG_PRIVATE, &cfp) != ERROR_ok) {
			return;
		}
		/*if (ts3Functions.getChannelClientList(serverConnectionHandlerID, (anyID)id, &queries) == ERROR_ok) {
			for (size_t i = 0; i < length(queries); i++)
			{
				ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, queries[i], CLIENT_TYPE, &cl_type);
				if (cl_type == 1) {
					mid = "";
				}
			}
		}
		else {
			return;
		}*/
		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "%s\nPhonetic Name: %s\nIcon ID: [color=darkgrey]%u[/color]\nForced Silence: [color=blue]%u[/color]\nPrivate Channel: [color=blue]%u[/color]\nQuery Clients:\n", buf, string, iconid, cfs, cfp);
		ts3Functions.freeMemory(string);
		//ts3Functions.freeMemory(queries);
		//ts3Functions.freeMemory(mid);
		break;
	case PLUGIN_CLIENT:
		ts3Functions.requestClientVariables(serverConnectionHandlerID, (anyID)id, NULL);
			now = time(NULL);
			time(&now);
			ts = *localtime(&now);
			strftime(buf, sizeof(buf), "%A, %d.%m.%Y %H:%M:%S", &ts);
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_TYPE, &cl_type) != ERROR_ok) {
			return;
		}
		else {
			if (cl_type == 0)
			{
				clienttype = "Client";
			}
			else if (cl_type == 1)
			{
				clienttype = "Query";
			}
			else
			{
				clienttype = "Unkown";
			}
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_COUNTRY, &clc) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME_PHONETIC, &string) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_UNREAD_MESSAGES, &unread) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_ICON_ID, &iconid) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_TALK_POWER, &talkpower) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_CHANNEL_GROUP_ID, &cgid) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_SERVERGROUPS, &sgids) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_NEEDED_SERVERQUERY_VIEW_POWER, &sqnvp) != ERROR_ok) {
			return;
		}
		//if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_LOGIN_NAME, &cln) != ERROR_ok) {
		//	return;
		//}
		//if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_LOGIN_PASSWORD, &clp) != ERROR_ok) {
		//	return;
		//}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_BADGES, &clb) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_UPLOADED, &mmbu) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_DOWNLOADED, &mmbd) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_UPLOADED, &tmbu) != ERROR_ok) {
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_DOWNLOADED, &tmbd) != ERROR_ok) {
			return;
		}
		/*if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, TEST, &teststr) != ERROR_ok) {
			return;
		}*/
		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "%s\nType: [COLOR=#1a2643]%s[/COLOR] from [color=darkgreen]%s[/color]\nPhonetic Name: [color=darkblue]%s[/color]\nBadges: %s\nIcon ID: [color=darkgrey]%u[/color]\nUnread Messages: [color=darkblue][b]%i[/b][/color]\nTalk Power: [color=darkgreen]%i[/color]\nNeeded ServerQuery View Power: [color=blue]%i[/color]\nServer Group IDs: [color=firebrick]%s[/color]\nChannel Group ID: [color=darkorange]%i[/color]\nMonthly Traffic: Up: [color=blue]%u[/color] B | Down: [color=red]%u[/color] B\nTotal Traffic: Up: [color=darkblue]%u[/color] B | Down: [color=firebrick]%u[/color] B", buf, clienttype, clc, string, clb, iconid, unread, talkpower, sqnvp, sgids, cgid, mmbu, mmbd, tmbu, tmbd);//Login Name: \"%s\"\nLogin PW: \"%s\"\n cln, clp,
		ts3Functions.freeMemory(string);
		ts3Functions.freeMemory(sgids);
		ts3Functions.freeMemory(clc);
		ts3Functions.freeMemory(clb);
		//ts3Functions.freeMemory(cln);
		//ts3Functions.freeMemory(clp);
		//ts3Functions.freeMemory(teststr);
		//ts3Functions.freeMemory(clienttype);
		break;
	default:
		data = NULL;
		return;
	}
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}


//void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID) {
//	if (requested == 1) {
//
//		
//
//		requested = 0;
//		ts3Functions.requestInfoUpdate(serverConnectionHandlerID,blub, id)
//	};
//}

/*
* Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
* the user manually disabled it in the plugin dialog.
* This function is optional. If missing, no autoload is assumed.
*/
int ts3plugin_requestAutoload() {
	return 1;  /* 1 = request autoloaded, 0 = do not request autoload */
}
//
///* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}
//
///* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);
//
///*
// * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
// * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
// * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
// */
enum {
		MENU_ID_GLOBAL_1
};
//
///*
// * Initialize plugin menus.
// * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
// * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
// * If plugin menus are not used by a plugin, do not implement this function or return NULL.
// */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	BEGIN_CREATE_MENUS(1);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_1, "About", "about.png");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "info.png");


}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	switch (type) {
		case PLUGIN_MENU_TYPE_GLOBAL:
			switch (menuItemID) {
				case MENU_ID_GLOBAL_1:
					MessageBoxA(0, PLUGIN_NAME " v" PLUGIN_VERSION " developed by " PLUGIN_AUTHOR " (" PLUGIN_CONTACT ")", "About " PLUGIN_NAME, MB_ICONINFORMATION);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}