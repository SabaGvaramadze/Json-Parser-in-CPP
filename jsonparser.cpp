#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

using namespace std;


enum json_exit_codes{
	JSON_VALID = 0,
	JSON_END,
	JSON_OBJECT_END,
	JSON_ARRAY_END,
	JSON_EXPECTED_KEY,
	JSON_EXPECTED_VALUE,
	JSON_VALUE_NOT_EXPECTED,
	JSON_COLON_NOT_EXPECTED,
	JSON_NO_KEY,
	JSON_INVALID_OBJECT,
	JSON_UNKNOWN_ERROR 
};

enum parser_states{
	QUOTE_OPEN=1,
	VACANT_KEY=2,
	VACANT_COMMA=4,
	EXPECT_VALUE=8
};


enum token_type{
	TOKEN_END = 0,
	TOKEN_SYMBOL,
	TOKEN_OBJECT,
	TOKEN_OBJECT_LEFT,
	TOKEN_OBJECT_RIGHT,
	TOKEN_ARRAY,
	TOKEN_ARRAY_LEFT,
	TOKEN_ARRAY_RIGHT,
	TOKEN_QUOTATION_MARK,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_NULL,
	TOKEN_COMMA,
	TOKEN_COLON,
	TOKEN_STRING,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_INVALID
};

typedef struct {
	token_type type;
	const char *text;
	size_t text_len;
}token;

typedef struct {
	char *content;
	long long content_len;
	size_t cursor;
	bool quote_open;
	token end_token;
	ifstream file;
	size_t read;
}lexer;

typedef struct{
	string key;
	uint32_t state;
}parser;

typedef struct{
	token tkn;
	void* val;
}json_var;

const char* token_type_tostring(token_type);
bool inline token_is_value(const token_type &tkn);
void json_print_object(const unordered_map<string,json_var> &data,const size_t indent = 0);
void json_print_arr(vector<json_var> &data,const size_t indent = 0);

lexer lexer_new(char *content,size_t content_len);
token lexer_next(lexer *l);
token lexer_next_quote(lexer *l);
void lexer_trim_left(lexer *l);
int8_t refill_content(lexer *l,token &tkn);

parser parser_new();
int8_t json_parse(lexer *l,unordered_map<string,json_var> &parsed_data,parser &json,string file_name);
int8_t json_parse_object(lexer *l,unordered_map<string,json_var> &parsed_data,parser &json);
int8_t json_parse_array(lexer *l,vector<json_var> &arr,parser &json);

int main(int argc,char **argv){
	string file_name = string(argv[1]);
	
	char *text = (char*)malloc(1024*1024+1);
	memset(text,0,1024*1024+1);
	lexer l = lexer_new(text,1024*1024);
	l.read = 0;
	l.file = ifstream(file_name);
	unordered_map<string,json_var> res;
	
	int result = JSON_VALID;
	parser json = parser_new();
	
	result = (int)json_parse(&l,res,json,file_name);
	cout << "EXIT_CODE: " << result << endl;

	
	
	return 0;

}



// !!WARNING!! token text will become invalid if the content of the lexer changes, but the token types are still valid
int8_t json_parse(lexer *l,unordered_map<string,json_var> &parsed_data,parser &json,string file_name){

	return json_parse_object(l,parsed_data,json);
}

int8_t json_parse_object(lexer *l,unordered_map<string,json_var> &parsed_data,parser &json){
	json.state |=VACANT_COMMA;
	bool vacant_key= json.state & VACANT_KEY,vacant_comma = json.state & VACANT_COMMA,expect_value = json.state & EXPECT_VALUE;
	token a;
	while(true){
	vacant_key= json.state & VACANT_KEY;
	vacant_comma = json.state & VACANT_COMMA;
	expect_value = json.state & EXPECT_VALUE;

	a = lexer_next(l);
	string key= json.key;
	switch(a.type){
		case TOKEN_END:
			break;
		case TOKEN_OBJECT_LEFT:
			if(expect_value){
				json.state ^= EXPECT_VALUE;
				parsed_data[key].tkn = a;
				parsed_data[key].tkn.type = TOKEN_OBJECT;
				parsed_data[key].val = (void*)(new unordered_map<string,json_var>);
				json.state |= VACANT_COMMA;
				int8_t exit_code = JSON_VALID;
				while(exit_code != JSON_OBJECT_END){
					exit_code = json_parse_object(l,*((unordered_map<string,json_var>*)parsed_data[key].val),json);
					if(exit_code != JSON_OBJECT_END)return exit_code;
				}
				break;
			}
			break;
		case TOKEN_OBJECT_RIGHT:
			return JSON_OBJECT_END;
		case TOKEN_ARRAY_LEFT:
			if(expect_value){
				json.state ^= EXPECT_VALUE;
				parsed_data[key].tkn = a;
				parsed_data[key].tkn.type = TOKEN_ARRAY;
				parsed_data[key].val= (void*)(new vector<json_var>);
				uint8_t res1 = json_parse_array(l,*((vector<json_var>*)parsed_data[key].val),json);
				if(res1 != JSON_ARRAY_END)return res1;
				break;
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_STRING:{
			if(expect_value) {
				parsed_data[key].tkn = a;
				parsed_data[key].val =  (void*)(new string(a.text,a.text_len));
				json.state ^= EXPECT_VALUE;
			}
			else{
				json.key.assign(a.text,a.text_len);
				json.state |= VACANT_KEY;
			}
			break;
			}
		case TOKEN_TRUE:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new bool);
				*((bool*)parsed_data[key].val) = true;
				break;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_FALSE:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new bool);
				*((bool*)parsed_data[key].val) = false;
				break;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_NULL:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new uint8_t);
				*((uint8_t*)parsed_data[key].val) = 0;
				break;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_COLON:
			if(vacant_key && vacant_comma){
				json.state |= EXPECT_VALUE;
				json.state ^= (VACANT_KEY | VACANT_COMMA);
				break;
			}
			return JSON_NO_KEY;
		case TOKEN_INT:
			if(expect_value){
				int val;
				parsed_data[key].tkn = a;
				try{
					val = stoi(string(a.text,a.text_len));
				}
				catch(const out_of_range){
					long long val2 = stoll(string(a.text,a.text_len));
					parsed_data[key].val = (void*)(new long long);
					*((long long*)parsed_data[key].val) = val2;
					json.state ^= EXPECT_VALUE;
					break;

				}
				parsed_data[key].val = (void*)(new int(val));
				json.state ^= EXPECT_VALUE;
				break;
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_FLOAT:
			if(expect_value){
				float val;
				double val2;
				parsed_data[key].tkn = a;
				try{
					val = stof(string(a.text,a.text_len));
				}
				catch( const out_of_range){
					val2 = stod(string(a.text,a.text_len));
					parsed_data[key].val = (void*)(new double(val2));
					json.state ^= EXPECT_VALUE;
					break;
				}
				parsed_data[key].val = (void*)(new float(val));
				json.state ^= EXPECT_VALUE;
				break;
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_COMMA:
			if(!expect_value){
				json.state |= VACANT_COMMA;
				break;
			}
			return JSON_COLON_NOT_EXPECTED;
			break;
		default:
			return JSON_UNKNOWN_ERROR;
	}
}
}
int8_t json_parse_array(lexer *l,vector<json_var> &arr,parser &json){
	json.state |= EXPECT_VALUE;
	
	token tkn;

	json_var value;
	while(true){
		tkn = lexer_next(l);
		if(tkn.type == TOKEN_ARRAY_RIGHT){
			if(json.state & EXPECT_VALUE)return JSON_EXPECTED_VALUE;
			return JSON_ARRAY_END;
		}
		else if(tkn.type == TOKEN_COMMA){
			if(json.state & EXPECT_VALUE)return JSON_EXPECTED_VALUE;
			json.state |= EXPECT_VALUE;
			continue;
		}
		else if(!token_is_value(tkn.type)){
			return JSON_EXPECTED_VALUE;
		}
		
		value.tkn = tkn;
		switch(tkn.type){
			case TOKEN_INT:
				int val;
				long long val2;
				try{
					val = stoi(string(tkn.text,tkn.text_len));
				}
				catch(const out_of_range){
					val2 = stoll(string(tkn.text,tkn.text_len));
					value.val = (void*)(new long long);
					*((long long*)value.val) = val2;
					break;
				}
				value.val = (void*)(new int(val));
				break;
			case TOKEN_FLOAT:
				float valf;
				double valf2;
				try{
					valf = stof(string(tkn.text,tkn.text_len));
				}
				catch( const out_of_range){
					valf2 = stod(string(tkn.text,tkn.text_len));
					value.val = (void*)(new double(valf2));
					break;
				}
				value.val = (void*)(new float(valf));
				break;
			case TOKEN_STRING:
				value.val =  (void*)(new string(tkn.text,tkn.text_len));
				break;
			case TOKEN_TRUE:
				value.val = (void*)(new bool);
				*((bool*)val) = true;
				break;
			case TOKEN_FALSE:
				value.val = (void*)(new bool);
				*((bool*)val) = false;
	 			break;
			case TOKEN_NULL:
				value.val = (void*)(new uint8_t);
				*((uint8_t*)value.val) = 0;
				break;
			case TOKEN_ARRAY_LEFT:
				value.tkn.type = TOKEN_ARRAY;
				value.val = (void*)(new vector<json_var>);
				json_parse_array(l,*((vector<json_var>*)value.val),json);
				break;
			case TOKEN_OBJECT_LEFT:
				value.tkn.type = TOKEN_OBJECT;
				value.val = (void*)(new unordered_map<string,json_var>);
				json.state ^= EXPECT_VALUE;
				uint8_t parse_res = json_parse_object(l,*((unordered_map<string,json_var>*)value.val),json);
				if(parse_res != JSON_VALID && parse_res !=JSON_OBJECT_END)return parse_res;
				arr.push_back(value);
				l->end_token=value.tkn;
				continue;
		}
		json.state ^= EXPECT_VALUE;
		arr.push_back(value);
		l->end_token=value.tkn;
	}
	return JSON_VALID;
}

void json_print_object(const unordered_map<string,json_var> &data,const size_t indent){
	for (const auto& [key, value] : data)
	{
		for(int j =0;j<indent;j++)cout << '	';
    		cout << key << ": " ;
		switch(value.tkn.type){
			case TOKEN_STRING:
				cout << *((string*)value.val);
				break;
			case TOKEN_INT:
				cout <<  *((int*)value.val);
				break;
			case TOKEN_FLOAT:
				cout << *((float*)value.val);
				break;
			case TOKEN_OBJECT:
				cout << endl;
				json_print_object(*((unordered_map<string,json_var>*)value.val),indent+1);
				break;
			case TOKEN_ARRAY:
				cout << endl;
				json_print_arr(*((vector<json_var>*)value.val),indent+1);
				break;
			default:
				cout << "NON VALUE TYPE: " << token_type_tostring(value.tkn.type) << " :: " << key ;
				break;
		}
		cout<< endl;
	}

}

void json_print_arr(vector<json_var> &data,const size_t indent){
	for(vector<json_var>::iterator i =  data.begin();i != data.end();i++){
		for(int j =0;j<indent;j++)cout << '	';
		switch(i->tkn.type){
		case TOKEN_STRING:
			cout << *((string*)i->val);
			break;
		case TOKEN_INT:
			cout <<  *((int*)i->val);
			break;
		case TOKEN_FLOAT:
			cout << *((float*)i->val);
			break;
		case TOKEN_OBJECT:
			cout << endl;
			json_print_object(*((unordered_map<string,json_var>*)i->val),indent+1);
			break;
		case TOKEN_ARRAY:
			cout << endl;
			json_print_arr(*((vector<json_var>*)i->val),indent+1);
			break;
		default:
			cout << "NON VALUE TYPE: " << token_type_tostring(i->tkn.type);
			break;
		}
	cout << endl;
	}
}

int8_t refill_content(lexer *l,token &tkn){

	
	
	memcpy(l->content,tkn.text,tkn.text_len);
	tkn.text= l->content;
	
	long long read_before = l->file.tellg();
	
	l->file.seekg(0,ios_base::end);
	long long file_size = l->file.tellg();
	l->file.seekg(read_before);


	l->file.read(l->content+tkn.text_len,min((long long)(l->content_len-tkn.text_len),file_size-read_before));
	long long read_after = l->file.tellg();
	
	l->cursor =tkn.text_len;
	
	l->read += read_after-read_before;
	if(read_after > read_before){
		l->content[l->cursor + read_after-read_before] = 0;
	}
	else if(read_after == read_before){
		return 0;
	}

	return 1;
	
}

inline bool token_is_value(const token_type &tkn){
	return !(tkn != TOKEN_STRING && tkn != TOKEN_INT && tkn != TOKEN_FLOAT && tkn != TOKEN_OBJECT_LEFT && tkn != TOKEN_ARRAY_LEFT && tkn != TOKEN_FALSE && tkn != TOKEN_TRUE);
}

// get rid of whitespaces
void lexer_trim_left(lexer *l){
	while(l->cursor < l->content_len && isspace(l->content[l->cursor])){
		l->cursor +=1;
	}
}

parser parser_new(){
	parser p;
	p.state = VACANT_COMMA;
	return p;
}

// generate lexer from data and data size
lexer lexer_new(char *content,size_t content_len){
	lexer l = {0};
	l.content = content;
	l.content_len = content_len;
	return l;
}


// get all text before a quote mark
token lexer_next_quote(lexer *l){
	token tkn;
	tkn.type = TOKEN_STRING;
	tkn.text = l->content + l->cursor;
	tkn.text_len =0;
	while(l->content[l->cursor] != '\"'){
		if(l->content[l->cursor] == 0){
			if(!refill_content(l,tkn)){
				tkn.type = TOKEN_END;
				return tkn;
			}
		}
		tkn.text_len += 1;
		l->cursor +=1;
	}
	return tkn;
}


// get next token
token lexer_next(lexer *l){
	lexer_trim_left(l);
	
	token tkn;
	tkn.text_len = 0;
	tkn.text = l->content + l->cursor;
	if(l->cursor >= l->content_len){
		if(!refill_content(l,tkn)){
			tkn.type = TOKEN_END;
			return tkn;
		}
	}
	
	//constants for comparison in the switch statement
	const char* truestr = "true";
	const char* falsestr = "false";
	const char* nullstr = "null";
	
	//single letter tokens, true, false and null
	switch(l->content[l->cursor]){
		case '{':
			l->cursor +=1;
			tkn.type = TOKEN_OBJECT_LEFT;
			tkn.text_len = 1;
			return tkn;
		case '}':
			l->cursor +=1;
			tkn.type = TOKEN_OBJECT_RIGHT;
			tkn.text_len = 1;
			return tkn;
		case '[':
			l->cursor +=1;
			tkn.type = TOKEN_ARRAY_LEFT;
			tkn.text_len = 1;
			return tkn;
		case ']':
			l->cursor +=1;
			tkn.type = TOKEN_ARRAY_RIGHT;
			tkn.text_len = 1;
			return tkn;
		case ':':
			l->cursor +=1;
			tkn.type = TOKEN_COLON;
			tkn.text_len = 1;
			return tkn;
		case ',':
			l->cursor +=1;
			tkn.type = TOKEN_COMMA;
			tkn.text_len = 1;
			return tkn;
		case '\"':
			l->cursor +=1;
			tkn = lexer_next_quote(l);
			l->cursor +=1;
			return tkn;
		case 't':
			tkn.text_len = 0;
			while(isalpha(l->content[l->cursor])){
				tkn.text_len +=1;
				if(l->content[l->cursor] == 0){
					if(!refill_content(l,tkn)){
						tkn.type = TOKEN_END;
						return tkn;
					}
					tkn.type = TOKEN_END;
					return tkn;
				}
				if(tkn.text_len>4 || truestr[tkn.text_len-1] != l->content[l->cursor]){
					tkn.type=TOKEN_INVALID;
					return tkn;
				}
				l->cursor +=1;
			}
			if(tkn.text_len != 4){
				tkn.type = TOKEN_INVALID;
				return tkn;
			}
			tkn.type = TOKEN_TRUE;
			return tkn;
		case 'f':
			tkn.text_len = 0;
			while(isalpha(l->content[l->cursor])){
				tkn.text_len +=1;
				if(l->content[l->cursor] == 0){
					if(!refill_content(l,tkn)){
						
						tkn.type = TOKEN_END;
						return tkn;
					}
					tkn.type = TOKEN_END;
					return tkn;
				}
				if(tkn.text_len>5 || falsestr[tkn.text_len-1] != l->content[l->cursor]){
					tkn.type=TOKEN_INVALID;
					return tkn;
				}
				l->cursor +=1;
			}
			if(tkn.text_len != 5){
				tkn.type = TOKEN_INVALID;
				return tkn;
			}
			tkn.type = TOKEN_FALSE;
			return tkn;
		case 'n':
			tkn.text_len = 0;
			while(isalpha(l->content[l->cursor])){
				tkn.text_len +=1;
				if(l->content[l->cursor] == 0){
					if(!refill_content(l,tkn)){
						tkn.type = TOKEN_END;
						return tkn;
					}
				}
				if(tkn.text_len>4 || nullstr[tkn.text_len-1] != l->content[l->cursor]){
					tkn.type=TOKEN_INVALID;
					return tkn;
				}
				l->cursor +=1;
			}
			if(tkn.text_len != 4){
				tkn.type = TOKEN_INVALID;
				return tkn;
			}
			tkn.type = TOKEN_NULL;
			return tkn;
		case 0:
			if(!refill_content(l,tkn)){
				tkn.type = TOKEN_END;
				return tkn;
			}
			tkn.type = TOKEN_END;
			tkn.text_len = 0;
			return tkn;
	}

	// get an integer or float
	if(isdigit(l->content[l->cursor]) || l->content[l->cursor] == '-'){
		token tkn;
		tkn.text = l->content + l->cursor;
		tkn.type = TOKEN_INT;
		tkn.text_len = 0;
		if(l->content[l->cursor] == 0){
			if(!refill_content(l,tkn)){
				tkn.type = TOKEN_INVALID;
				return tkn;
			}
		}
		while(isdigit(l->content[l->cursor]) || l->content[l->cursor] == '.'){
			if(l->content[l->cursor] == '.'){
				if(tkn.type == TOKEN_FLOAT){
					tkn.type = TOKEN_INVALID;
					break;
				}
				tkn.type = TOKEN_FLOAT;
			}
			tkn.text_len+=1;
			l->cursor+= 1;
			if(l->content[l->cursor] == 0){
				if(!refill_content(l,tkn)){
					tkn.type = TOKEN_INVALID;
					return tkn;
				}
			}

		}
		return tkn;
	}
	tkn.type = TOKEN_INVALID;
	tkn.text_len = 1;
	l->cursor +=1;
	return tkn;
}

const char* token_type_tostring(token_type type){
	switch(type){
		case TOKEN_END:
			return "END TOKEN";
		case TOKEN_OBJECT:
			return "OBJECT TOKEN";
		case TOKEN_OBJECT_LEFT:
			return "LEFT OBJECT TOKEN";
		case TOKEN_OBJECT_RIGHT:
			return "RIGHT OBJECT TOKEN";
		case TOKEN_ARRAY:
			return "ARRAY TOKEN";
		case TOKEN_ARRAY_LEFT:
			return "LEFT ARRAY TOKEN";
		case TOKEN_ARRAY_RIGHT:
			return "RIGHT ARRAY TOKEN";
		case TOKEN_QUOTATION_MARK:
			return "QUOTATION MARK TOKEN";
		case TOKEN_TRUE:
			return "TRUE TOKEN";
		case TOKEN_FALSE:
			return "FALSE TOKEN";
		case TOKEN_NULL:
			return "NULL TOKEN";
		case TOKEN_COMMA:
			return "COMMA TOKEN";
		case TOKEN_COLON:
			return "COLON TOKEN";
		case TOKEN_STRING:
			return "STRING TOKEN";
		case TOKEN_INT:
			return "INT TOKEN";
		case TOKEN_FLOAT:
			return "FLOAT TOKEN";
		case TOKEN_INVALID:
			return "INVALID TOKEN";
		default:
			return "NO TOKEN FOUND";
	}
}
