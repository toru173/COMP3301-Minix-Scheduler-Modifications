//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <cstdlib>
#include <fstream>
#include <vector>

#include "config.hpp"
#include "config_file.hpp"
#include "env.hpp"
#include "fs.hpp"
#include "parser.hpp"

namespace impl = tools::config_file;
namespace detail = tools::config_file::detail;

namespace {

typedef std::map< std::string, std::string > vars_map;

namespace atf_config {

static const tools::parser::token_type eof_type = 0;
static const tools::parser::token_type nl_type = 1;
static const tools::parser::token_type text_type = 2;
static const tools::parser::token_type dblquote_type = 3;
static const tools::parser::token_type equal_type = 4;
static const tools::parser::token_type hash_type = 5;

class tokenizer : public tools::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        tools::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim('=', equal_type);
        add_delim('#', hash_type);
        add_quote('"', dblquote_type);
    }
};

} // namespace atf_config

class config_reader : public detail::atf_config_reader {
    vars_map m_vars;

    void
    got_var(const std::string& var, const std::string& name)
    {
        m_vars[var] = name;
    }

public:
    config_reader(std::istream& is) :
        atf_config_reader(is)
    {
    }

    const vars_map&
    get_vars(void)
        const
    {
        return m_vars;
    }
};

template< class K, class V >
static
void
merge_maps(std::map< K, V >& dest, const std::map< K, V >& src)
{
    for (typename std::map< K, V >::const_iterator iter = src.begin();
         iter != src.end(); iter++)
        dest[(*iter).first] = (*iter).second;
}

static
void
merge_config_file(const tools::fs::path& config_path,
                  vars_map& config)
{
    std::ifstream is(config_path.c_str());
    if (is) {
        config_reader reader(is);
        reader.read();
        merge_maps(config, reader.get_vars());
    }
}

static
std::vector< tools::fs::path >
get_config_dirs(void)
{
    std::vector< tools::fs::path > dirs;
    dirs.push_back(tools::fs::path(tools::config::get("atf_confdir")));
    if (tools::env::has("HOME"))
        dirs.push_back(tools::fs::path(tools::env::get("HOME")) / ".atf");
    return dirs;
}

} // anonymous namespace

detail::atf_config_reader::atf_config_reader(std::istream& is) :
    m_is(is)
{
}

detail::atf_config_reader::~atf_config_reader(void)
{
}

void
detail::atf_config_reader::got_var(
    const std::string& var __attribute__((__unused__)),
    const std::string& val __attribute__((__unused__)))
{
}

void
detail::atf_config_reader::got_eof(void)
{
}

void
detail::atf_config_reader::read(void)
{
    using tools::parser::parse_error;
    using namespace atf_config;

    std::pair< size_t, tools::parser::headers_map > hml =
        tools::parser::read_headers(m_is, 1);
    tools::parser::validate_content_type(hml.second,
        "application/X-atf-config", 1);

    tokenizer tkz(m_is, hml.first);
    tools::parser::parser< tokenizer > p(tkz);

    for (;;) {
        try {
            tools::parser::token t = p.expect(eof_type, hash_type, text_type,
                                              nl_type,
                                              "eof, #, new line or text");
            if (t.type() == eof_type)
                break;

            if (t.type() == hash_type) {
                (void)p.rest_of_line();
                t = p.expect(nl_type, "new line");
            } else if (t.type() == text_type) {
                std::string name = t.text();

                t = p.expect(equal_type, "equal sign");

                t = p.expect(text_type, "word or quoted string");
                ATF_PARSER_CALLBACK(p, got_var(name, t.text()));

                t = p.expect(nl_type, hash_type, "new line or comment");
                if (t.type() == hash_type) {
                    (void)p.rest_of_line();
                    t = p.expect(nl_type, "new line");
                }
            } else if (t.type() == nl_type) {
            } else
                std::abort();
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }

    ATF_PARSER_CALLBACK(p, got_eof());
}

vars_map
impl::merge_configs(const vars_map& lower,
                    const vars_map& upper)
{
    vars_map merged = lower;
    merge_maps(merged, upper);
    return merged;
}

vars_map
impl::read_config_files(const std::string& test_suite_name)
{
    vars_map config;

    const std::vector< tools::fs::path > dirs = get_config_dirs();
    for (std::vector< tools::fs::path >::const_iterator iter = dirs.begin();
         iter != dirs.end(); iter++) {
        merge_config_file((*iter) / "common.conf", config);
        merge_config_file((*iter) / (test_suite_name + ".conf"), config);
    }

    return config;
}
