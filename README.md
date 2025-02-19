JSON Parser in C++

A JSON parser written in C++ that parses a file and stores the data in std::unsorted_maps and std::vectors
the data is stored as json_var, which containts val and tkn variables. tkn is the token information(most important being tkn.type), using which you can cast the val(stored as void*) into the appropriate type.
there also are functions json_print_object() and json_print_array() which allow you to print a unsorted_map or a vector
