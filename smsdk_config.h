#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

/* Basic information exposed publicly */
#define SMEXT_CONF_NAME			"AwesomeHvH extension"
#define SMEXT_CONF_DESCRIPTION	"HvH related fixes"
#define SMEXT_CONF_VERSION		"1.0.2"
#define SMEXT_CONF_AUTHOR		"Nixer1337"
#define SMEXT_CONF_URL			"https://www.unknowncheats.me/forum/counterstrike-global-offensive/487084-awesomehvh-server-plugin-legacy-desync.html"
#define SMEXT_CONF_LOGTAG		"AwesomeHvH"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

/**
 * @brief Exposes plugin's main interface.
 */
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

 /**
  * @brief Sets whether or not this plugin required Metamod.
  * NOTE: Uncomment to enable, comment to disable.
  */
#define SMEXT_CONF_METAMOD

  /** Enable interfaces you want to use here by uncommenting lines */
  //#define SMEXT_ENABLE_FORWARDSYS
  //#define SMEXT_ENABLE_HANDLESYS
  //#define SMEXT_ENABLE_PLAYERHELPERS
  //#define SMEXT_ENABLE_DBMANAGER
#define SMEXT_ENABLE_GAMECONF
#define SMEXT_ENABLE_MEMUTILS
#define SMEXT_ENABLE_GAMEHELPERS
//#define SMEXT_ENABLE_TIMERSYS
//#define SMEXT_ENABLE_THREADER
//#define SMEXT_ENABLE_LIBSYS
//#define SMEXT_ENABLE_MENUS
//#define SMEXT_ENABLE_ADTFACTORY
//#define SMEXT_ENABLE_PLUGINSYS
//#define SMEXT_ENABLE_ADMINSYS
//#define SMEXT_ENABLE_TEXTPARSERS
//#define SMEXT_ENABLE_USERMSGS
//#define SMEXT_ENABLE_TRANSLATOR
//#define SMEXT_ENABLE_ROOTCONSOLEMENU

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
