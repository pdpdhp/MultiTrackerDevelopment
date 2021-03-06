#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <map>
#include <string>
#include "array1.h"
#include "vec.h"

struct ParseTree
{
   std::multimap<std::string, ParseTree> branches;
   std::map<std::string, double> numbers;
   std::map<std::string, std::string> strings;
   std::map<std::string, ElTopo::Array1d> vectors;

   std::vector<const ParseTree*> get_multi_branch(const std::string& name) const;
   const ParseTree* get_branch(const std::string& name) const;
   bool get_number(const std::string& name, double& result) const;
   bool get_int(const std::string& name, int& result) const;
   bool get_string(const std::string& name, std::string& result) const;
   const ElTopo::Array1d* get_vector(const std::string& name) const;
   bool get_vec2d(const std::string& name, ElTopo::Vec2d& v) const;
   bool get_vec3d(const std::string& name, ElTopo::Vec3d& v) const;
};

std::ostream& operator<<(std::ostream& out, const ParseTree& tree);

// return true if no errors occur
bool parse_stream(std::istream& input, ParseTree& tree);

#endif

