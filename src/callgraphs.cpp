/**
 * \file callgraphs.cpp
 * \author Laurent Georget
 * \date 2015-03-03
 * \brief Entry point of the plugin
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include <gcc-plugin.h>

#include <system.h>
#include <coretypes.h>
#include <tree.h>
#include <tree-pass.h>
#include <intl.h>

#include <tm.h>
#include <diagnostic.h>

#include <function.h>
#include <tree-iterator.h>
#include <gimple.h>

#include <dumpfile.h>

#include "callgrapher.h"


extern "C" {
	/**
	 * \brief Must-be-defined value for license compatibility
	 */
	int plugin_is_GPL_compatible;
	static unsigned int find_calls(void);

	/**
	 * \brief Basic information about the plugin
	 */
	static struct plugin_info myplugin_info =
	{
		"0.1", // version
		"KayrebtCallgraphs: a GCC plugin to build a database of callers/callees from a source code", //help
	};

	/**
	 * \brief Plugin version information
	 */
	static struct plugin_gcc_version myplugin_version =
	{
		"4.8", //basever
		"2017-03-28", // datestamp
		"alpha", // devphase
		"", // revision
		"", // configuration arguments
	};

	/**
	 * \brief Definition of the new pass added by the plugin
	 */
	struct gimple_opt_pass find_calls_pass =
	{
		{
			GIMPLE_PASS, /* type */
			"findcalls", /* name */
			OPTGROUP_NONE, /* optinfo_flags */
			NULL, /* gate */
			find_calls, /* execute */
			NULL, /* sub */
			NULL, /* next */
			0, /* static_pass_number */
			TV_NONE, /* tv_id */
			PROP_cfg, /* properties_required */
			0, /* properties_provided */
			0, /* properties_destroyed */
			0, /* todo_flags_start */
			0, /* todo_flags_finish */
		}
	};

}

/**
 * @brief The SQLITE3 database (-fplugin-arg-kayrebt_callgraphs-dbfile=...)
 */
const char* dbFile = "";

/**
 * \brief Plugin entry point
 * \param plugin_info the command line options passed to the plugin
 * \param version the plugin version information
 * \return 0 if everything went well, -1 if the plugin is incompatible with
 * the active version of GCC
 */
extern "C" int plugin_init (struct plugin_name_args *plugin_info,
		struct plugin_gcc_version *version)
{
	// Check that GCC base version starts with "4.8"
	if (strncmp(version->basever,myplugin_version.basever,3) != 0) {
		std::cerr << "Sadly, the KayrebtCallgraphs GCC plugin is only "
			"available for GCC 4.8 for now." << std::endl;
		return -1; // incompatible
	}

	struct register_pass_info find_calls_pass_info = {
		.pass				= &find_calls_pass.pass,
		.reference_pass_name		= "cfg", //before inlining!
		.ref_pass_instance_number	= 1,
		.pos_op				= PASS_POS_INSERT_AFTER
	};

	int argc = plugin_info->argc;
	struct plugin_argument *argv = plugin_info->argv;
	const char* plugin_name = plugin_info->base_name;

	for (int i = 0; i < argc; ++i) {
		if (!strcmp (argv[i].key, "dbfile")) {
			if (argv[i].value) {
				dbFile = argv[i].value;
			} else {
				warning (0, G_("option '-fplugin-arg-%s-dbfile'"
							" ignored (missing arguments)"),
						plugin_name);
			}
		}
		else {
			warning (0, G_("plugin %qs: unrecognized argument %qs ignored"),
					plugin_name, argv[i].key);
		}
	}
	register_callback(plugin_info->base_name,
			  PLUGIN_PASS_MANAGER_SETUP,
			  NULL,
			  &find_calls_pass_info);
	return 0;
}

/**
 * \brief Evaluate paths
 *
 * If there is a compilation error, no graph is produced.
 * \return 0 even if there is an error, in order to build as many graphs as
 * possible without making GCC crash, except if the error is global
 */
extern "C" unsigned int find_calls()
{
	// If there were errors during compilation,
	// let GCC handle the exit.
	if (errorcount || sorrycount)
		return 0;

	Callgrapher c(dbFile);
	c.findAllCalls();

	return 0;
}


