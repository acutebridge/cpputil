/*
Copyright 2013 eric schkufza

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef CPPUTIL_SRC_ARGS_H
#define CPPUTIL_SRC_ARGS_H

#include <algorithm>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "src/singleton.h"

namespace cpputil {

class Arg;

class Args {
	friend class Singleton<Args>;
	friend class Arg;

	public:
		typedef std::vector<Arg*>::iterator error_iterator;
		typedef std::vector<std::string>::const_iterator unrecognized_iterator;
		typedef std::vector<std::string>::const_iterator anonymous_iterator;

		static void read(int argc, char** argv, std::ostream& os = std::cout);
		static std::string usage(size_t indent = 0);

		static bool error();
		static error_iterator error_begin();
		static error_iterator error_end();

		static bool unrecognized();
		static unrecognized_iterator unrecognized_begin();
		static unrecognized_iterator unrecognized_end();

		static anonymous_iterator anonymous_begin();
		static anonymous_iterator anonymous_end();

		static bool good();
		static bool fail();

	private:
		std::map<char, Arg*> args_;
		std::vector<Arg*> errors_;
		std::vector<std::string> unrecognized_;
		std::vector<std::string> anonymous_;
};

class Arg {
	friend class Args;

	public:
		virtual ~Arg() = 0;

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout) = 0;

		template <typename T>
		class Reader {
			public:
				void operator()(std::istream& is, T& t);
		};

		template <typename T>
		class Writer {
			public:
				void operator()(std::ostream& os, const T& t);
		};

	protected:
		char opt_;
		std::string alt_;

		Arg(char opt);

		Arg& description(const std::string& desc);
		Arg& alternate(const std::string& alt);
		Arg& usage(const std::string& u);

		virtual option to_longopt() const = 0;
		virtual std::string to_optstr() const = 0;

		static char** copy(int argc, char** argv);

	private:
		std::string desc_;
		std::string usage_;

		size_t width(size_t indent) const;
};

class FlagArg : public Arg {
	public:
		virtual ~FlagArg();

		static FlagArg& create(char opt);

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

		operator bool();
		bool value();

		FlagArg& description(const std::string& desc);
		FlagArg& alternate(const std::string& alt);
		FlagArg& usage(const std::string& u);

	protected:	
		virtual option to_longopt() const;
		virtual std::string to_optstr() const;

	private:
		bool val_;

		FlagArg(char opt); 
};

template <typename T, typename P = Arg::Reader<T>>
class ValueArg : public Arg {
	public:
		virtual ~ValueArg();

		static ValueArg& create(char opt);

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

		operator T&();
		T& value();

		ValueArg& description(const std::string& desc);
		ValueArg& alternate(const std::string& alt);
		ValueArg& usage(const std::string& u);

		ValueArg& default_val(const T& def);
		ValueArg& parse_error(const std::string& pe);

	protected:	
		virtual option to_longopt() const;
		virtual std::string to_optstr() const;

	private:
		T val_;
		std::string parse_error_;	
		P parser_;

		ValueArg(char opt);
};

template <typename T, typename P = Arg::Reader<T>>
class FileArg : public Arg {
	public:
		virtual ~FileArg();

		static FileArg& create(char opt);

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

		operator T&();
		T& value();

		FileArg& description(const std::string& desc);
		FileArg& alternate(const std::string& alt);
		FileArg& usage(const std::string& u);

		FileArg& default_path(const std::string& path);
		FileArg& default_val(const T& def);
		FileArg& parse_error(const std::string& pe);
		FileArg& file_error(const std::string& fe);

	protected:	
		virtual option to_longopt() const;
		virtual std::string to_optstr() const;

	private:
		std::string path_;
		T val_;
		P parser_;
		std::string parse_error_;
		std::string file_error_;

		FileArg(char opt);
};

inline void Args::read(int argc, char** argv, std::ostream& os) {
	auto& args = Singleton<Args>::get();
	const auto argv_copy = Arg::copy(argc, argv);

	std::vector<option> longopts;
	std::string opts = "-";
	std::set<std::string> known;

	for ( const auto& arg : args.args_ ) {
		if ( arg.second->alt_ != std::string("") ) {
			longopts.push_back(arg.second->to_longopt());
			known.insert(std::string("--") + arg.second->alt_);
		}

		std::ostringstream oss;
		oss << "-" << arg.second->opt_;
		
		known.insert(oss.str());
		opts += arg.second->to_optstr();
	}
	longopts.push_back(option{0,0,0,0});

	optind = opterr = 0;
	while ( true ) {
		const auto next = argv_copy[optind == 0 ? 1 : optind];
		const auto c = getopt_long(argc, argv_copy, opts.c_str(), 
				longopts.data(), 0);
		if ( c == -1 ) {
			break;
		} else if ( c == 1 ) {
			args.anonymous_.push_back(next);
		} else if ( c == '?' && known.find(next) == known.end() ) {
			args.unrecognized_.push_back(next);
		}
	}

	for ( auto& arg : args.args_ )
		if ( !arg.second->read(argc, argv, os) )
			args.errors_.push_back(arg.second);
}

inline std::string Args::usage(size_t indent) {
	auto& args = Singleton<Args>::get();

	size_t max_width = 0;
	for ( const auto& arg : args.args_ )
		max_width = std::max(max_width, arg.second->width(indent));

	std::ostringstream oss;
	for ( const auto& arg : args.args_ ) {
		for ( size_t j = 0; j < indent; ++j )
			oss << " ";
		oss << "-" << arg.second->opt_ << " ";
		if ( arg.second->alt_ != "" )
			oss << "--" << arg.second->alt_ << " ";
		oss << arg.second->usage_ << " ";
	
		for ( size_t j = arg.second->width(indent); j < max_width; ++j )
			oss << ".";

		oss << "... ";
		oss << arg.second->desc_;

		oss << std::endl;
	}

	return oss.str();
}

inline bool Args::error() {
	return !Singleton<Args>::get().errors_.empty();
}

inline Args::error_iterator Args::error_begin() {
	return Singleton<Args>::get().errors_.begin();
}

inline Args::error_iterator Args::error_end() {
	return Singleton<Args>::get().errors_.end();
}

inline bool Args::unrecognized() {
	return !Singleton<Args>::get().unrecognized_.empty();
}

inline Args::unrecognized_iterator Args::unrecognized_begin() {
	return Singleton<Args>::get().unrecognized_.begin();
}

inline Args::unrecognized_iterator Args::unrecognized_end() {
	return Singleton<Args>::get().unrecognized_.end();
}

inline Args::anonymous_iterator Args::anonymous_begin() {
	return Singleton<Args>::get().anonymous_.begin();
}

inline Args::anonymous_iterator Args::anonymous_end() {
	return Singleton<Args>::get().anonymous_.end();
}

inline bool Args::good() {
	return !error() && !unrecognized();
}

inline bool Args::fail() {
	return !good();
}

inline Arg::~Arg() {
}

template <typename T>
inline void Arg::Reader<T>::operator()(std::istream& is, T& t) {
	is >> t;
}

template <typename T>
inline void Arg::Writer<T>::operator()(std::ostream& os, const T& t) {
	os << t;
}

inline Arg::Arg(char opt) 
		: opt_{opt}, alt_{""}, desc_{"???"}, usage_{"???"} { 
	Singleton<Args>::get().args_[opt_] = this;
}

inline Arg& Arg::description(const std::string& desc) { 
	desc_ = desc; 
	return *this; 
}

inline Arg& Arg::alternate(const std::string& alt) { 
	alt_ = alt; 
	return *this; 
}

inline Arg& Arg::usage(const std::string& u) { 
	usage_ = u;
	return *this; 
}

inline char** Arg::copy(int argc, char** argv) {
	static std::vector<char*> buffer;
	for ( auto a : buffer )
		free(a);
	buffer.clear();

	for ( auto i = 0; i < argc; ++i )
		buffer.push_back(strdup(argv[i]));
	return buffer.data();
}

inline size_t Arg::width(size_t indent) const {
	// "<indent>-c "
	size_t w = indent+3; 
	// "--alt "
	if ( alt_ != "" )
		w += (2+alt_.length()+1);
	// "usage "
	w += (usage_.length()+1);

	return w;
}

inline FlagArg::~FlagArg() { 
}

inline FlagArg& FlagArg::create(char opt) {
	auto fa = new FlagArg{opt};
	fa->usage("");
	fa->description("Flag Arg");

	return *fa;
}

inline bool FlagArg::read(int argc, char** argv, std::ostream& os) { 
	const auto argv_copy = Arg::copy(argc, argv);

	std::vector<option> longopts;
	if ( alt_ != std::string("") )
		longopts.push_back(to_longopt());
	longopts.push_back({0,0,0,0});
	const auto optstr = std::string("-") + to_optstr();

	optind = opterr = 0;
	while ( true ) {
		const auto c = getopt_long(argc, argv_copy, optstr.c_str(), 
				longopts.data(), 0);
		if ( c == -1 ) {
			break;
		} else if ( c == opt_ ) {
			val_ = true;
			break;
		}
	}

	return true; 
}

inline FlagArg::operator bool() { 
	return val_; 
}

inline bool FlagArg::value() {
	return val_;
}

inline FlagArg& FlagArg::description(const std::string& desc) { 
	Arg::description(desc); 
	return *this; 
}

inline FlagArg& FlagArg::alternate(const std::string& alt) { 
	Arg::alternate(alt); 
	return *this; 
}

inline FlagArg& FlagArg::usage(const std::string& u) { 
	Arg::usage(u); 
	return *this; 
}

inline option FlagArg::to_longopt() const {
	return {alt_.c_str(), no_argument, 0, opt_};
}

inline std::string FlagArg::to_optstr() const {
	std::ostringstream oss;
	oss << opt_;
	return oss.str();
}

inline FlagArg::FlagArg(char opt) 
		: Arg{opt}, val_{false} { 
}

template <typename T, typename P>
inline ValueArg<T, P>::~ValueArg() { 
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::create(char opt) {
	auto va = new ValueArg{opt};

	std::ostringstream oss;
	oss << "Error (-" << opt << "): Unable to parse input value!" << std::endl;

	va->usage("<arg>");
	va->description("Value Arg");
	va->parse_error(oss.str());

	return *va;
}

template <typename T, typename P>
inline bool ValueArg<T, P>::read(int argc, char** argv, std::ostream& os) { 
	const auto argv_copy = Arg::copy(argc, argv);

	std::vector<option> longopts;
	if ( alt_ != std::string("") )
		longopts.push_back(to_longopt());
	longopts.push_back({0,0,0,0});
	const auto optstr = std::string("-") + to_optstr();

	char* res = 0;
	optind = opterr = 0;
	while ( true ) {
		const auto c = getopt_long(argc, argv_copy, optstr.c_str(), 
				longopts.data(), 0);
		if ( c == -1 ) {
			break;
		} else if ( c == opt_ ) {
			res = optarg;
			break;
		}
	}
	if ( res == 0 )
		return true;

	T temp = T();
	std::istringstream iss(res);
	parser_(iss, temp);

	if ( iss.fail() ) {
		os << parse_error_;
		return false;
	}

	val_ = temp;
	return true;
}

template <typename T, typename P>
inline ValueArg<T, P>::operator T&() { 
	return val_; 
}

template <typename T, typename P>
inline T& ValueArg<T, P>::value() {
	return val_;
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::description(const std::string& desc) { 
	Arg::description(desc); 
	return *this; 
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::alternate(const std::string& alt) { 
	Arg::alternate(alt); 
	return *this; 
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::usage(const std::string& u) { 
	Arg::usage(u); 
	return *this; 
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::default_val(const T& t) { 
	val_ = t; 
	return *this; 
}

template <typename T, typename P>
inline ValueArg<T, P>& ValueArg<T, P>::parse_error(const std::string& pe) { 
	parse_error_ = pe; 
	return *this; 
}

template <typename T, typename P>
inline option ValueArg<T, P>::to_longopt() const {
	return {alt_.c_str(), required_argument, 0, opt_};
}

template <typename T, typename P>
inline std::string ValueArg<T, P>::to_optstr() const {
	std::ostringstream oss;
	oss << opt_ << ":";
	return oss.str();
}

template <typename T, typename P>
inline ValueArg<T, P>::ValueArg(char opt) 
		: Arg{opt} {
}

template <typename T, typename P>
inline FileArg<T, P>::~FileArg() {
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::create(char opt) {
	auto fa = new FileArg{opt};

	std::ostringstream oss1;
	oss1 << "Error (-" << opt << "): Unable to read input value!" << std::endl;
	std::ostringstream oss2;
	oss2 << "Error (-" << opt << "): Unable to read input file!" << std::endl;

	fa->usage("<path>");
	fa->description("File Arg");
	fa->parse_error(oss1.str());
	fa->file_error(oss2.str());

	return *fa;
}

template <typename T, typename P>
inline bool FileArg<T, P>::read(int argc, char** argv, std::ostream& os) {
	const auto argv_copy = Arg::copy(argc, argv);

	std::vector<option> longopts;
	if ( alt_ != std::string("") )
		longopts.push_back(to_longopt());
	longopts.push_back({0,0,0,0});
	const auto optstr = std::string("-") + to_optstr();

	char* res = 0;
	optind = opterr = 0;
	while ( true ) {
		const auto c = getopt_long(argc, argv_copy, optstr.c_str(), 
				longopts.data(), 0);
		if ( c == -1 ) {
			break;
		} else if ( c == opt_ ) {
			res = optarg;
			break;
		}
	}
	if ( res != 0 )
		path_ = res;

	std::ifstream ifs(path_);
	if ( !ifs.is_open() ) {
		os << file_error_;
		return false;
	}

	T temp = T();
	parser_(ifs, temp);

	if ( ifs.fail() ) {
		os << parse_error_;
		return false;
	}

	val_ = temp;
	return true;
}

template <typename T, typename P>
inline FileArg<T, P>::operator T&() {
	return val_;
}

template <typename T, typename P>
inline T& FileArg<T, P>::value() {
	return val_;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::description(const std::string& desc) {
	Arg::description(desc);
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::alternate(const std::string& alt) {
	Arg::alternate(alt);
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::usage(const std::string& u) {
	Arg::usage(u);
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::default_path(const std::string& path) {
	path_ = path;
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::default_val(const T& def) {
	val_ = def;
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::parse_error(const std::string& pe) {
	parse_error_ = pe;
	return *this;
}

template <typename T, typename P>
inline FileArg<T, P>& FileArg<T, P>::file_error(const std::string& fe) {
	file_error_ = fe;
	return *this;
}

template <typename T, typename P>
inline option FileArg<T, P>::to_longopt() const {
	return {alt_.c_str(), required_argument, 0, opt_};
}

template <typename T, typename P>
inline std::string FileArg<T, P>::to_optstr() const {
	std::ostringstream oss;
	oss << opt_ << ":";
	return oss.str();
}

template <typename T, typename P>
inline FileArg<T, P>::FileArg(char opt) 
		: Arg{opt} {
}

} // namespace cpputil

#endif
