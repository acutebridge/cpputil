#ifndef CPPUTIL_SRC_ARGS_H
#define CPPUTIL_SRC_ARGS_H

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <iostream>
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

		static bool read(int argc, char** argv, std::ostream& os = std::cout);
		static std::string usage();

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
		std::vector<Arg*> args_;
		std::vector<Arg*> errors_;
		std::vector<std::string> unrecognized_;
		std::vector<std::string> anonymous_;

		Args() = default;
};

class Arg {
	friend class Args;

	public:
		virtual ~Arg() = 0;

	protected:
		Arg(char opt);

		Arg& description(const std::string& desc);
		Arg& alternate(const std::string& alt);
		Arg& usage(const std::string& u);

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout) = 0;

		char opt_;
		std::string alt_;

	private:
		std::string desc_;
		std::string usage_;

		size_t width() const;
};

class FlagArg : public Arg {
	public:
		virtual ~FlagArg();

		static FlagArg& create(char opt);

		operator bool();

		FlagArg& description(const std::string& desc);
		FlagArg& alternate(const std::string& alt);
		FlagArg& usage(const std::string& u);

	protected:
		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

	private:
		bool val_;

		FlagArg(char opt); 
};

template <typename T>
class ValueArg : public Arg {
	public:
		virtual ~ValueArg();

		static ValueArg& create(char opt);

		operator T&();

		ValueArg& description(const std::string& desc);
		ValueArg& alternate(const std::string& alt);
		ValueArg& usage(const std::string& u);

		ValueArg& default_val(const T& def);
		ValueArg& parse_error(const std::string parse);

	protected:
		T val_;
		std::string parse_;	

		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

		ValueArg(char opt);
};

template <typename T>
class FileArg : public ValueArg<std::string> {
	public:
		virtual ~FileArg();

		static FileArg& create(char opt);

		operator T&();

		FileArg& description(const std::string& desc);
		FileArg& alternate(const std::string& alt);
		FileArg& usage(const std::string& u);

		FileArg& default_val(const T& def);
		FileArg& parse_error(const std::string parse);
		FileArg& file_error(const std::string file);

	protected:
		virtual bool read(int argc, char** argv, std::ostream& os = std::cout);

	private:
		std::string file_;
		T file_val_;

		FileArg(char opt);
};

inline bool Args::read(int argc, char** argv, std::ostream& os) {
	auto args = Singleton<Args>::get();
	for ( const auto& arg : args.args_ )
		if ( !arg->read(argc, argv, os) )
			args.errors_.push_back(arg);
}

inline std::string Args::usage() {
	auto args = Singleton<Args>::get();

	size_t max_width = 0;
	for ( const auto arg : args.args_ )
		max_width = std::max(max_width, arg->width());

	std::ostringstream oss;
	for ( size_t i = 0, ie = args.args_.size(); i < ie; ++i ) {
		const auto arg = args.args_[i];

		oss << "  -" << arg->opt_ << " ";
		if ( arg->alt_ != "" )
			oss << "--" << arg->alt_ << " ";
		oss << arg->usage_ << " ";
	
		for ( size_t j = arg->width(); j < max_width; ++j )
			oss << ".";

		oss << "... ";
		oss << arg->desc_;

		if ( i+1 != ie )
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
	return Singleton<Args>::get().anonymous_end();
}

inline bool Args::good() {
	return !error() && !unrecognized();
}

inline bool Args::fail() {
	return !good();
}

inline Arg::~Arg() {
}

inline Arg::Arg(char opt) 
		: opt_{opt}, alt_{""}, desc_{"???"}, usage_{"???"} { 
	Singleton<Args>::get().args_.push_back(this);
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

inline size_t Arg::width() const {
	// "  -c "
	size_t w = 5; 
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

inline FlagArg::operator bool() { 
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

inline bool FlagArg::read(int argc, char** argv, std::ostream& os) { 
	option longopts[] =	{option{0,0,0,0}, option{0,0,0,0}};
	if ( alt_ != "" )
		longopts[0] = option{alt_.c_str(), no_argument, 0, opt_};
	const auto opts = std::string("") + opt_;

	auto c = 0;

	const auto ignore1 = freopen("/dev/null", "w", stderr);
	optind = 1;
	while ( (c = getopt_long(argc, argv, opts.c_str(), longopts, 0)) != -1 ) {
		if ( c == opt_ ) {
			val_ = true;
			break;
		}
	}
	const auto ignore2 = freopen("/dev/tty", "w", stderr);

	return true; 
}

inline FlagArg::FlagArg(char opt) 
		: Arg{opt}, val_{false} { 
}

template <typename T>
inline ValueArg<T>::~ValueArg() { 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::create(char opt) {
	auto va = new ValueArg{opt};

	std::ostringstream oss;
	oss << "Error (-" << opt << "): Unable to parse input value!" << std::endl;

	va->usage("<arg>");
	va->description("Value Arg");
	va->parse_error(oss.str());

	return *va;
}

template <typename T>
inline ValueArg<T>::operator T&() { 
	return val_; 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::description(const std::string& desc) { 
	Arg::description(desc); 
	return *this; 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::alternate(const std::string& alt) { 
	Arg::alternate(alt); 
	return *this; 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::usage(const std::string& u) { 
	Arg::usage(u); 
	return *this; 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::default_val(const T& t) { 
	val_ = t; 
	return *this; 
}

template <typename T>
inline ValueArg<T>& ValueArg<T>::parse_error(const std::string parse) { 
	parse_ = parse; 
	return *this; 
}

template <typename T>
inline bool ValueArg<T>::read(int argc, char** argv, std::ostream& os) { 
	option longopts[] =	{option{0,0,0,0}, option{0,0,0,0}};
	if ( alt_ != "" )
		longopts[0] = option{alt_.c_str(), required_argument, 0, opt_};
	const auto opts = std::string("") + opt_ + std::string(":");

	auto c = 0;
	char* res = 0;

	const auto ignore1 = freopen("/dev/null", "w", stderr);
	optind = 1;
	while ( (c = getopt_long(argc, argv, opts.c_str(), longopts, 0)) != -1 ) {
		if ( c == opt_ ) {
			res = optarg;
			break;
		}
	}
	const auto ignore2 = freopen("/dev/tty", "w", stderr);

	if ( res == 0 )
		return true;

	std::istringstream iss(res);
	iss >> val_;
	if ( iss.fail() )
		os << parse_;

	return !iss.fail();
}

template <typename T>
inline ValueArg<T>::ValueArg(char opt) 
		: Arg{opt} {
}

template <typename T>
inline FileArg<T>::~FileArg() {
}

template <typename T>
inline FileArg<T>& FileArg<T>::create(char opt) {
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

template <typename T>
inline FileArg<T>::operator T&() {
	return file_val_;
}

template <typename T>
inline FileArg<T>& FileArg<T>::description(const std::string& desc) {
	ValueArg<std::string>::description(desc);
	return *this;
}

template <typename T>
inline FileArg<T>& FileArg<T>::alternate(const std::string& alt) {
	ValueArg<std::string>::alternate(alt);
	return *this;
}

template <typename T>
inline FileArg<T>& FileArg<T>::usage(const std::string& u) {
	ValueArg<std::string>::usage(u);
	return *this;
}

template <typename T>
inline FileArg<T>& FileArg<T>::parse_error(const std::string parse) {
	ValueArg<std::string>::parse_error(parse);
	return *this;
}

template <typename T>
inline FileArg<T>& FileArg<T>::default_val(const T& def) {
	file_val_ = def;
	return *this;
}

template <typename T>
inline FileArg<T>& FileArg<T>::file_error(const std::string file) {
	file_ = file;
	return *this;
}

template <typename T>
inline bool FileArg<T>::read(int argc, char** argv, std::ostream& os) {
	if ( !ValueArg<std::string>::read(argc, argv, os) )
		return false;

	if ( val_ == "" )
		return true;

	std::ifstream ifs(val_);
	if ( !ifs.is_open() ) {
		os << file_;
		return false;
	}

	ifs >> file_val_;
	if ( ifs.fail() ) {
		os << parse_;
		return false;
	}

	return true;
}

template <typename T>
inline FileArg<T>::FileArg(char opt) 
		: ValueArg<std::string>{opt} {
}

} // namespace cpputil

#endif
