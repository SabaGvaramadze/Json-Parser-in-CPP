#include <iostream>
#include <map>
#include <unistd.h>
#include <stdint.h>
using namespace std;


enum json_exit_codes{
	JSON_VALID = 0,
	JSON_END,
	JSON_OBJECT_END,
	JSON_EXPECTED_KEY,
	JSON_VALUE_NOT_EXPECTED,
	JSON_COLON_NOT_EXPECTED,
	JSON_NO_KEY,
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
	const char *content;
	size_t content_len;
	size_t cursor;
	size_t line;
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

lexer lexer_new(const char *content,size_t content_len);
token lexer_next(lexer *l);
token lexer_next_quote(lexer *l);
void lexer_trim_left(lexer *l);

parser parser_new();
uint8_t json_parse(lexer *l,map<string,json_var> &parsed_data,parser &json);

int main(int argc,char **argv){
	const char *text = "{   \"hello\":32 , \"name\"    : \"st./m:;$#upid ass\",\"angle\":      69.3325, \"obj\" : {\"hi\":\"bye\",\"goodbeatch\":69420}, \"arr\":[1,2,3,4,5,6,7,8,9,0]}";
	
	lexer l = lexer_new(text,strlen(text));
	map<string,json_var> res;
	int result = JSON_VALID;
	parser json = parser_new();
	while(result != JSON_END){
		result = (int)json_parse(&l,res,json);
	}
	for (const auto& [key, value] : res)
	{
    		std::cout << key << ": ";
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
				for (const auto& [key1,value1] : *((map<string,json_var>*)value.val)){
					cout  << '	' << key1 << ": ";
					switch(value1.tkn.type){
						case TOKEN_STRING:
							cout << *((string*)value1.val);
							break;
						case TOKEN_INT:
							cout <<  *((int*)value1.val);
							break;
						case TOKEN_FLOAT:
							cout << *((float*)value1.val);
							break;
						default:
							cout << "type: " << token_type_tostring(value.tkn.type);
							break;
					}
					cout << endl;
				}
				break;
				
			default:
				cout << "type: " << token_type_tostring(value.tkn.type);
		}
		cout << endl;
	}


}



// !!WARNING!! token text will become invalid if the content of the lexer changes, but the token types are still valid
uint8_t json_parse(lexer *l,map<string,json_var> &parsed_data,parser &json){
	bool vacant_key= json.state & VACANT_KEY,vacant_comma = json.state & VACANT_COMMA,expect_value = json.state & EXPECT_VALUE;
	token a = lexer_next(l);
	string key= json.key;
	cout << endl << token_type_tostring(a.type) << endl;
	if(a.type == TOKEN_END){
		return JSON_END;
	}
	switch(a.type){
		case TOKEN_END:
			return JSON_END;
		case TOKEN_OBJECT_LEFT:
			if(expect_value){
				json.state ^= EXPECT_VALUE;
				parsed_data[key].tkn = a;
				parsed_data[key].tkn.type = TOKEN_OBJECT;
				parsed_data[key].val = (void*)(new map<string,json_var>);
				json.state |= VACANT_COMMA;
				while(json_parse(l,*((map<string,json_var>*)parsed_data[key].val),json) != JSON_OBJECT_END);
				return JSON_VALID;
			}
			return JSON_VALID;
		case TOKEN_OBJECT_RIGHT:
			return JSON_OBJECT_END;
		case TOKEN_ARRAY_LEFT:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].tkn.typ = TOKEN_ARRAY;
				parsed_data[key].val= (void*)(new vector<json_var>);
				a = lexer_next(l);
				while(a.tkn.type != TOKEN_ARRAY_RIGHT || a.tkn.type != TOKEN_END || a.tkn.type != TOKEN_INVALID){
					if(a.tkn.type != TOKEN_COMMA){
						json_var array_element;
						array_element.tkn = a;
						switch(a.tkn.type){
							case TOKEN_INT:
								
						}
						array_element
						(*((vector<json_var>*)parsed_data[key].val)).push_back(
					}
				}
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_STRING:{
			cout << "STRING: " << string(a.text,a.text_len) << endl;
			if(a.type == TOKEN_END)return JSON_END;
			if(expect_value && a.type == TOKEN_STRING){
				cout << "string value key: " << key << endl;
				parsed_data[key].tkn = a;
				parsed_data[key].val =  (void*)(new string(a.text,a.text_len));
				json.state ^= EXPECT_VALUE;
			}
			else{
				json.key.assign(a.text,a.text_len);
				json.state |= VACANT_KEY;
			}
			return JSON_VALID;
			}
		case TOKEN_TRUE:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new bool);
				*((bool*)parsed_data[key].val) = true;
				return JSON_VALID;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_FALSE:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new bool);
				*((bool*)parsed_data[key].val) = false;
				return JSON_VALID;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_NULL:
			if(expect_value){
				parsed_data[key].tkn = a;
				parsed_data[key].val = (void*)(new uint8_t);
				*((uint8_t*)parsed_data[key].val) = 0;
				return JSON_VALID;
			}
			else{
				return JSON_EXPECTED_KEY;
			}
		case TOKEN_COLON:
			if(vacant_key && vacant_comma){
				json.state |= EXPECT_VALUE;
				json.state ^= (VACANT_KEY | VACANT_COMMA);
				return JSON_VALID;
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
					return JSON_VALID;

				}
				parsed_data[key].val = new int(val);
				cout << "key: " << key << endl;
				cout << "INT: " << val << endl;
				json.state ^= EXPECT_VALUE;
				return JSON_VALID;
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_FLOAT:
			if(expect_value){
				float val;
				parsed_data[key].tkn = a;
				try{
					val = stof(string(a.text,a.text_len));
				}
				catch( const out_of_range){
					float val2 = stod(string(a.text,a.text_len));
					parsed_data[key].val = (void*)(new double(val2));
					json.state ^= EXPECT_VALUE;
					return JSON_VALID;
				}
				parsed_data[key].val = (void*)(new float(val));
				cout << "key: " << key << endl;
				cout << "FLOAT: " << val;
				json.state ^= EXPECT_VALUE;
				return JSON_VALID;
			}
			return JSON_VALUE_NOT_EXPECTED;
		case TOKEN_COMMA:
			if(!expect_value){
				json.state |= VACANT_COMMA;
				return JSON_VALID;
			}
			return JSON_COLON_NOT_EXPECTED;
			break;
		default:
			return JSON_UNKNOWN_ERROR;
	}
	return JSON_VALID;
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
lexer lexer_new(const char *content,size_t content_len){
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
			tkn.type = TOKEN_END;
			return tkn;
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
	tkn.text = l->content + l->cursor;
	if(l->cursor >= l->content_len){
		tkn.type = TOKEN_END;
		return tkn;
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
					tkn.type = TOKEN_END;
					return tkn;
				}
				if(tkn.text_len>4 || nullstr[tkn.text_len-1] != l->content[l->cursor]){
		;			tkn.type=TOKEN_INVALID;
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
	}

	// get an integer or float
	if(isdigit(l->content[l->cursor]) || l->content[l->cursor] == '-'){
		token tkn;
		tkn.text = l->content + l->cursor;
		tkn.type = TOKEN_INT;
		tkn.text_len = 0;
		while(isdigit(l->content[l->cursor]) || l->content[l->cursor] == '.'){
			if(l->content[l->cursor] == '.'){
				if(tkn.type == TOKEN_FLOAT){
					tkn.type = TOKEN_INVALID;
					break;
				}
				tkn.type = TOKEN_FLOAT;
			}
			tkn.text_len+=1;
			l->cursor+=1;

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
			return "ARRAY OBJECT";
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
