#pragma once

#include <string>
#include <unordered_map>

#include "Permissions.hpp"
#include "utils.hpp"

enum XDGDir {
    XDG_DATA_HOME,
    XDG_CACHE_HOME,
    XDG_CONFIG_HOME,
    XDG_RUNTIME_DIR,

    // NOTE: Must be the final element.
    __XDG_DIR_NUM_TYPES,
};

/**
 * XDG Base directory specification tools.
 *
 * To use it, initialize an XDG object, and query it as follows:
 *   XDG xdg;
 *   xdg.path(XDG_CONFIG_HOME, "relative", "from", "XDG", "dir");
 *   xdg.path(XDG_DATA_HOME, "relative", "from", "XDG", "dir");
 *   xdg.path(XDG_CACHE_HOME, "relative", "from", "XDG", "dir");
 *   xdg.path(XDG_RUNTIME_DIR, "relative", "from", "XDG", "dir")
 *
 * This class does not look for the following:
 *   - XDG_CONFIG_DIRS
 *   - XDG_DATA_DIRS
 *   - user-dirs.dirs, i.e XDG_DESKTOP_DIR, XDG_DOWNLOAD_DIR, etc.
 */
class XDG {
    std::string dirs[__XDG_DIR_NUM_TYPES];
    std::string appname;

public:
    /**
     * @throws SystemError if unable to find (or create) the following XDG directories:
     *                     - $HOME (fallback: /home/$(whoami))
     *                     - $XDG_CONFIG_HOME (fallback: $HOME/.config)
     *                     - $XDG_DATA_HOME (fallback: $HOME/.local/share)
     *                     - $XDG_CACHE_HOME (fallback: $HOME/.cache)
     *                     - $XDG_RUNTIME_DIR (fallback: $XDG_DATA_HOME:$HOME/.xdg-runtime-dir)
     *                       - $XDG_DATA_HOME will only be used if it has permissions 0700
     *
     * Therefore if XDG::XDG() returns without incident you can count on DATA_HOME,
     * CONFIG_HOME, and CACHE_HOME being defined, which should be the minimum.
     *
     * The following XDG directories should be defined, but are not guaranteed
     * by this class to exist after instantiation:
     *
     * In the case of SystemError I recommend letting it bubble back up to main()
     * and to then display a helpful error message, it should never happen
     * unless something has gone terribly wrong (and user action is required to
     * correct the error.)
     */
    XDG(const std::string& name) noexcept(false);
    virtual ~XDG();

    /**
     * Retrieve an environment variable as a string.
     *
     * @param name Name of environment variable.
     * @param fallback String to fall back on if the name isn't found.
     * @return The value of `name` in the environment.
     */
    inline static std::string envString(const std::string &name,
                                        const std::string &fallback) noexcept {
        char *str = getenv(name.c_str());
        if (str == nullptr)
            return fallback;
        return std::string(str);
    }

    /**
     * See envString(const std::string &name, const std::string &fallback) for
     * full documentation.
     *
     * Sets `fallback` to "".
     */
    inline static std::string envString(const std::string &name) noexcept {
        return envString(name, "");
    }

    /**
     * See mkdirIfNotExists(const std::string& dir, mode_t um) for full documentation.
     *
     * Sets `um` to 0700.
     */
    static inline bool mkdirIfNotExists(const std::string& dir) {
        return mkdirIfNotExists(dir, 0700);
    }

    /**
     * Create a directory if it did not exist already.
     *
     * If a new directory is created, this is logged to syslog as a warning.
     *
     * @param dir The directory path.
     * @param um File creation permissions bitfield (see stat.st_mode).
     * @return Whether a new directory was created.
     * @throws SystemError if unable to create a new directory, or if
     *                     a file already existed at `dir` but it was
     *                     not a directory.
     */
    static bool mkdirIfNotExists(const std::string& dir, mode_t um);

    /**
     * See mkPathNotExists(const std::string& dir, mode_t um) for full documentation.
     *
     * Sets `um` to 0700.
     */
    static inline int mkPathIfNotExists(const std::string& path) {
        return mkPathIfNotExists(path, 0700);
    }

    /**
     * Walks along a path to create every directory along it.
     *
     * It is assumed that the all parts of the path are directories, the final
     * part should not be a file.
     *
     * @param path Path separated by forward slashes ('/'), can be absolute or relative.
     * @param um File creation permissions bitfield (see stat.st_mode).
     * @return The number of directories created.
     * @throws SystemError See mkdirIfNotExists.
     */
    static int mkPathIfNotExists(const std::string& path, mode_t um);

    /**
     * Check if a path exists and is a directory.
     */
    static bool isDir(const std::string& path);

    template <class... Ts>
    inline std::string path(XDGDir basedir, Ts... rest) noexcept {
        return pathJoin(dirs[basedir], this->appname, rest...);
    }

    template <class... Ts>
    inline void mkpath(mode_t um, XDGDir basedir,  Ts... rest) noexcept {
        mkPathIfNotExists(path(basedir, rest...), um);
    }

    template <class... Ts>
    inline ChDir cd(XDGDir basedir, Ts... parts) {
        return ChDir(path(basedir, parts...));
    }

    template <class... Ts>
    inline void mkfifo(Ts... parts) {
        ::mkfifo(path(XDG_RUNTIME_DIR, parts...).c_str(), 0700);
    }
};
