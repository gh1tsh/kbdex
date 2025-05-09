/* =============================================================================
 *
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 *
 * BSD 2-Clause License
 *
 * Copyright (c) 2019, Jonas Møller
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =============================================================================
 */

#include "Permissions.hpp"
#include <functional>
#include <regex>
#include <unordered_map>

using namespace std;

std::string
Permissions::fmtPermissions(unsigned num) noexcept
{
        stringstream ret;
        string       bitstr[] = { "r", "w", "x" };
        for (unsigned mask = S_IRUSR, i = 0; mask >= S_IXOTH; mask >>= 1, i++)
                ret << ((num & mask) ? bitstr[i % 3] : "-");
        return ret.str();
}

Permissions::Collection::Collection(const struct stat *stbuf)
{
        mode              = stbuf->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
        auto [pwd, pwbuf] = getuser(uid = stbuf->st_uid);
        (void)pwbuf;
        user_name          = string(pwd->pw_name);
        auto [grp, grpbuf] = getgroup(gid = stbuf->st_gid);
        (void)grpbuf;
        group_name     = string(grp->gr_name);
        mode_mask      = S_IRWXU | S_IRWXG | S_IRWXO;
        file_type_char = fileTypeChar(stbuf);
}

Permissions::Collection::Collection(const std::string &perm)
{
        const static regex rx("([dpbclsf])"
                              "([r\\.-][w\\.-][x\\.-][r\\.-][w\\.-][x\\.-][r\\.-][w\\.-][x\\.-])"
                              " +"
                              "(~|\\*|[a-z_-]+)"
                              ":"
                              "(\\*|[a-z_-]+)");

        smatch cm;
        if (regex_search(perm, cm, rx)) {
                if (cm.size() < 5)
                        throw std::runtime_error("Not enough matches.");

                file_type_char = cm[1].str();
                user_name      = cm[3].str();
                if (user_name == "*") {
                        uid = -1;
                } else {
                        auto [pwd, pwbuf] = (user_name == "~") ? getuser() : getuser(user_name);
                        (void)pwbuf;
                        if (user_name == "~")
                                user_name = string(pwd->pw_name);
                        uid = pwd->pw_uid;
                }
                group_name = cm[4].str();
                if (group_name == "*") {
                        gid = -1;
                } else {
                        auto [grp, grbuf] = getgroup(group_name);
                        (void)grbuf;
                        gid = grp->gr_gid;
                }

                const string perm = cm[2].str();

                if (perm.size() != 9)
                        throw std::runtime_error("Expected permission string to be 9 characters.");

                mode_mask = 0;
                mode      = 0;
                for (unsigned mask = S_IRUSR, i = 0; mask >= S_IXOTH; mask >>= 1, i++) {
                        if (perm[i] == '.')
                                continue;
                        if (perm[i] != '-')
                                mode |= mask;
                        mode_mask |= mask;
                }

        } else {
                throw std::runtime_error("Illegal syntax in permission description string");
        }
}

Permissions::Collection
Permissions::describeFile(const std::string &path)
{
        struct stat stbuf;
        if (stat(path.c_str(), &stbuf) == -1)
                throw SystemError("Error in stat(): ", errno);
        return Collection(&stbuf);
}

std::string
Permissions::fileTypeChar(const struct stat *stbuf)
{
        if (S_ISDIR(stbuf->st_mode))
                return "d";
        if (S_ISFIFO(stbuf->st_mode))
                return "p";
        if (S_ISBLK(stbuf->st_mode))
                return "b";
        if (S_ISCHR(stbuf->st_mode))
                return "c";
        if (S_ISLNK(stbuf->st_mode))
                return "l";
        if (S_ISSOCK(stbuf->st_mode))
                return "s";
        if (S_ISREG(stbuf->st_mode))
                return "f";
        return "?";
}

Permissions::Collection
Permissions::parsePermissions(const std::string &perm)
{
        return Collection(perm);
}

std::string
Permissions::fmtPermissions(struct stat &stbuf) noexcept
{
        stringstream ret;
        string       rwx   = fmtPermissions(reinterpret_cast<unsigned>(stbuf.st_mode));
        auto [grp, grpbuf] = getgroup(stbuf.st_gid);
        (void)(grpbuf);
        auto [usr, usrbuf] = getuser(stbuf.st_uid);
        (void)(usrbuf);
        ret << rwx << " " << string(usr->pw_name) << ":" << grp->gr_name;
        return ret.str();
}

bool
Permissions::checkFile(const std::string &path, const std::string &mode)
{
        Collection expect = parsePermissions(mode);
        Collection have   = describeFile(path);
        bool       eq     = expect == have;
        if (!eq)
                syslog(LOG_ERR,
                       "Require %s but got %s on file: %s",
                       expect.fmt().c_str(),
                       have.fmt().c_str(),
                       path.c_str());
        return eq;
}
