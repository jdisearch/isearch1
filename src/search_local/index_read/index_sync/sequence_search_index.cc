/*
 * =====================================================================================
 *
 *       Filename:  sequence_search_index.h
 *
 *    Description:  sequence search index definition.
 *
 *        Version:  1.0
 *        Created:  09/08/2020
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhulin, shzhulin3@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */

#include "sequence_search_index.h"
#include "mutexlock.h"
#include "log.h"
#include "../index_tbl_op.h"
#include "../key_format.h"
#include <algorithm>
#include <iomanip>

#define MIN(x,y) ((x)<=(y)?(x):(y))

static const char *cache_configfile = "../conf/cache.conf";
const char *table_configfile = "../conf/table.conf";
const char *TABLE_NAME       = "keyword_index_data";
const char *INDEX_SYMBOL     = "00";
const char *MAX_BORDER_SYMBOL = "10";
const char *MIN_BORDER_SYMBOL = "00";

static string gen_dtc_key_string(string appid, string type, uint32_t key_type, double key) {
	log_debug("fieldtype:%d , key:%f " , key_type , key);
	KeyFormat::UnionKey o_keyinfo_vet;
    o_keyinfo_vet.push_back(std::make_pair(key_type , std::to_string(key)));
	std::string s_format_key = KeyFormat::Encode(o_keyinfo_vet);

#if 0
	KeyFormat::UnionKey o_output_vet;
	o_output_vet.push_back(std::make_pair(key_type , ""));
	KeyFormat::Decode(s_format_key , o_output_vet);
	for (int i = 0; i < o_output_vet.size(); i++){
		log_error("decode string:%s", o_output_vet[i].second.c_str());
	}
#endif
	std::stringstream stream_key;
	stream_key << appid << "#" << type << "#" << s_format_key;
	return stream_key.str();
}

static string gen_dtc_key_string(string appid, string type, string key) {
	stringstream ss;
	ss << appid << "#" << type << "#" << key;
	return ss.str();
}

SequenceSetIndex::SequenceSetIndex()
{
	_RwMutex = new pthread_rwlock_t;
	pthread_rwlock_init(_RwMutex,NULL);
}

void SequenceSetIndex::GetRangeIndex(uint32_t range_type, InvertIndexEntry &startEntry, InvertIndexEntry &endEntry, std::vector<InvertIndexEntry>& resultEntry)
{
	if (range_type == RANGE_GELE) {
		GetRangeIndexGELE(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GE) {
		GetRangeIndexGE(startEntry, resultEntry);
	}
	else if (range_type == RANGE_LE) {
		GetRangeIndexLE(endEntry, resultEntry);
	}
	else if (range_type == RANGE_GTLT) {
		GetRangeIndexGTLT(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GTLE) {
		GetRangeIndexGTLE(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GELT) {
		GetRangeIndexGELT(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GT) {
		GetRangeIndexGT(startEntry, resultEntry);
	}
	else if (range_type == RANGE_LT) {
		GetRangeIndexLT(startEntry, resultEntry);
	}
}

void SequenceSetIndex::GetRangeIndexGELE(InvertIndexEntry &begin_key, const InvertIndexEntry &end_key, std::vector<InvertIndexEntry>& entry)
{
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it == _SearchIndexs.end()) {
			return;
		}
		else {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			if (set_it != it->second.end()) {
				for(; set_it != it->second.end() && set_it->LE(end_key); set_it++){
					entry.push_back(*set_it);
				}
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexGE(InvertIndexEntry& begin_key, std::vector<InvertIndexEntry>& entry) {
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			for (; set_it != it->second.end(); set_it++) {
				entry.push_back(*set_it);
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexLE(InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry) {
	end_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(end_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), end_key);
			for (; set_it != it->second.begin(); set_it--) {
				entry.push_back(*set_it);
			}
			if (it->second.size() > 0) {
				entry.push_back(*it->second.begin());
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexGTLT(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry) {
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			for (; set_it != it->second.end() && set_it->LT(end_key); set_it++) {
				if ((*set_it).EQ(begin_key)) {
					continue;
				}
				entry.push_back(*set_it);
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexGTLE(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry) {
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			for (; set_it != it->second.end() && set_it->LE(end_key); set_it++) {
				if ((*set_it).EQ(begin_key)) {
					continue;
				}
				entry.push_back(*set_it);
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexGELT(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry) {
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			for (; set_it != it->second.end() && set_it->LT(end_key); set_it++) {
				entry.push_back(*set_it);
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexGT(InvertIndexEntry& begin_key, std::vector<InvertIndexEntry>& entry) {
	begin_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(begin_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
			for (; set_it != it->second.end(); set_it++) {
				if ((*set_it).EQ(begin_key)) {
					continue;
				}
				entry.push_back(*set_it);
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexLT(InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry) {
	end_key._InvertIndexDocId = "";
	std::string appid_field = GetIndexAppidField(end_key);
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), end_key);
			for (; set_it != it->second.begin(); set_it--) {
				if ((*set_it).EQ(end_key)) {
					continue;
				}
				entry.push_back(*set_it);
			}
			if (it->second.size() > 0) {
				entry.push_back(*it->second.begin());
			}
		}
	}
}

void SequenceSetIndex::GetScoreByCacheSet(const CacheQueryInfo &query_info, const set<string>& valid_docs, SkipList& skipList, map<string, uint32_t>& valid_version)
{
	std::ostringstream oss_index;
	oss_index << query_info.appid << "_" << query_info.sort_field;
	std::string appid_field = oss_index.str();
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it != _SearchIndexs.end()) {
			uint32_t i = 0;
			std::set<InvertIndexEntry> index_set= it->second;
			if(query_info.sort_type == SORT_FIELD_ASC){
				for(std::set<InvertIndexEntry>::iterator set_it = index_set.begin(); set_it != index_set.end(); set_it++){
					InvertIndexEntry entry = *set_it;
					if(i >= query_info.page_index * query_info.page_size){
						break;
					}
					if(valid_docs.find(entry._InvertIndexDocId) != valid_docs.end()){
						bool is_valid = false;
						g_IndexInstance.DocValid(query_info.appid, entry._InvertIndexDocId, is_valid);
						if(is_valid){
							i++;
							skipList.InsertNode(entry._InvertIndexKey, entry._InvertIndexDocId.c_str());
							valid_version.insert(make_pair(entry._InvertIndexDocId, entry._InvertIndexDocVersion));
						}
					}
				}
			} else {
				for(std::set<InvertIndexEntry>::reverse_iterator set_it = index_set.rbegin(); set_it != index_set.rend(); set_it++){
					InvertIndexEntry entry = *set_it;
					if(i >= query_info.page_index * query_info.page_size){
						break;
					}
					if(valid_docs.find(entry._InvertIndexDocId) != valid_docs.end()){
						bool is_valid = false;
						g_IndexInstance.DocValid(query_info.appid, entry._InvertIndexDocId, is_valid);
						if(is_valid){
							i++;
							skipList.InsertNode(entry._InvertIndexKey, entry._InvertIndexDocId.c_str());
							valid_version.insert(make_pair(entry._InvertIndexDocId, entry._InvertIndexDocVersion));
						}
					}
				}
			}
		}
	}
}

void SequenceSetIndex::GetScoreByCacheSetSearchAfter(const CacheQueryInfo &query_info, const set<string>& valid_docs, SkipList& skipList, map<string, uint32_t>& valid_version)
{
	std::ostringstream oss_index;
	oss_index << query_info.appid << "_" << query_info.sort_field;
	std::string appid_field = oss_index.str();
	{
		ReadLock rdlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if(it != _SearchIndexs.end()){
			InvertIndexEntry begin_key;
			begin_key._InvertIndexKey = atof(query_info.last_score.c_str());
			begin_key._InvertIndexDocId = query_info.last_id;
			uint32_t i = 0;
			if(query_info.sort_type == SORT_FIELD_ASC){
				std::set<InvertIndexEntry >::iterator set_it = std::upper_bound(it->second.begin(), it->second.end(), begin_key);
				for (; set_it != it->second.end(); set_it++) {
					if(i >= query_info.page_size){
						break;
					}
					InvertIndexEntry entry = *set_it;
					if(valid_docs.find(entry._InvertIndexDocId) != valid_docs.end()){
						bool is_valid = false;
						g_IndexInstance.DocValid(query_info.appid, entry._InvertIndexDocId, is_valid);
						if(is_valid){
							i++;
							skipList.InsertNode(entry._InvertIndexKey, entry._InvertIndexDocId.c_str());
							valid_version.insert(make_pair(entry._InvertIndexDocId, entry._InvertIndexDocVersion));
						}
					}
				}
			} else {
				std::set<InvertIndexEntry >::iterator set_it_old = std::lower_bound(it->second.begin(), it->second.end(), begin_key);
				std::set<InvertIndexEntry>::reverse_iterator set_it(set_it_old);
				for (; set_it != it->second.rend(); set_it++) {
					if(i >= query_info.page_size){
						break;
					}
					InvertIndexEntry entry = *set_it;
					if(valid_docs.find(entry._InvertIndexDocId) != valid_docs.end()){
						bool is_valid = false;
						g_IndexInstance.DocValid(query_info.appid, entry._InvertIndexDocId, is_valid);
						if(is_valid){
							i++;
							skipList.InsertNode(entry._InvertIndexKey, entry._InvertIndexDocId.c_str());
							valid_version.insert(make_pair(entry._InvertIndexDocId, entry._InvertIndexDocVersion));
						}
					}
				}
			}
		}
	}
}

void SequenceSetIndex::GetRangeIndexInTerminal(RANGTYPE range_type, const InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, const TerminalQryCond& query_cond, std::vector<InvertIndexEntry>& entry){

}

bool SequenceSetIndex::InsertIndex(const InvertIndexEntry & entry)
{
	std::string appid_field = GetIndexAppidField(entry);
	std::set<InvertIndexEntry> invert_index_entrys; 
	{
		WriteLock wrlock(_RwMutex);
		if (_SearchIndexs.find(appid_field) == _SearchIndexs.end()) {
			invert_index_entrys = std::set<InvertIndexEntry>();
			_SearchIndexs[appid_field] = invert_index_entrys;
		}
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		it->second.insert(entry);
	}
	
	return true;
}

bool SequenceSetIndex::DeleteIndex(const InvertIndexEntry &entry)
{
	std::string appid_field = GetIndexAppidField(entry);
	{
		WriteLock wrlock(_RwMutex);
		std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.find(appid_field);
		if (it == _SearchIndexs.end()) {
			return true;
		}
		else {
			std::set<InvertIndexEntry >::iterator set_it = std::lower_bound(it->second.begin(), it->second.end(), entry);
			if (set_it != it->second.end()) {
				while (set_it != it->second.end() && set_it->EQ(entry)) {
					it->second.erase(set_it);
					set_it++;
				}
			}		
		}
	}
	return false;
}

void SequenceSetIndex::DumpIndex()
{
	for (std::map<std::string, std::set<InvertIndexEntry> >::iterator it = _SearchIndexs.begin(); it != _SearchIndexs.end(); it++) {
		log_error("appid_field [%s]", it->first.c_str());
		for (std::set<InvertIndexEntry >::iterator set_it = it->second.begin(); set_it != it->second.end(); set_it++) {		
			log_error("index is  [%s]", set_it->ToString().c_str());
		}
	}
}

SearchRocksDBIndex::SearchRocksDBIndex(){
	uds_client = new UDSClient();	
}

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型
	char * strs = new char[str.length() + 1] ; //不要忘了
	strcpy(strs, str.c_str()); 
 
	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());
 
	char *p = strtok(strs, d);
	while(p) {
		string s = p; //分割得到的字符串转换为string类型
		res.push_back(s); //存入结果数组
		p = strtok(NULL, d);
	}
	return res;
}

std::string getPath(const char *bind_addr){
	string s = "/tmp/domain_socket/rocks_direct_20000.sock";
	return s;
}

void SearchRocksDBIndex::buildTableFieldMap(CConfig* _DTCTableConfig){
	int fieldCount = _DTCTableConfig->GetIntVal("TABLE_DEFINE", "FieldCount", 0);
  
	log_debug("buildTableFieldMap start fieldCount is %d",fieldCount);
	for(int i = 0; i < fieldCount; i++){
		string fieldName = "FIELD";

		std::stringstream ss;
		ss << (i+1);
		fieldName.append(ss.str());
		const char *field_name = _DTCTableConfig->GetStrVal((char*)fieldName.data(), "FieldName");
		int field_type = _DTCTableConfig->GetIntVal((char*)fieldName.data(), "FieldType", 4);
		struct TableField *table_field = new TableField();
		table_field->index = i;
		table_field->type = field_type;
		table_field_map.insert(pair<string, TableField*>(field_name, table_field));
		log_debug("field_name is %s, index is %d type is %d", field_name, i, field_type);
		table_field_vec.push_back(table_field);
	}
}



bool SearchRocksDBIndex::Init(){
	CConfig* _DTCTableConfig = new CConfig();
	if (table_configfile == NULL || _DTCTableConfig->ParseConfig(table_configfile, "Machine1")) {
		log_error("no table config or config file [%s] is error", table_configfile);
		return false;
	}

	CConfig* _DTCCacheConfig = new CConfig();

	if ( _DTCCacheConfig->ParseConfig(cache_configfile, "search_cache")) {
		log_error("no cache config or config file [%s] is error", cache_configfile);
		return false;
	}
  
	//读取配置文件
	const char *bind_addr = _DTCCacheConfig->GetStrVal("search_cache", "BindAddr");
	if (bind_addr == NULL) {
		log_error("has no BindAddr ");
		return false;
	}

	
	buildTableFieldMap(_DTCTableConfig);

	//获取端口号 拼接path地址字符串 进行unix socket连接
	string path = getPath(bind_addr);
	bool conn_result = uds_client->Connect((char*)path.c_str());
	return conn_result;
}


int SearchRocksDBIndex::getFieldIndex(const char *fieldName){
	map<string, struct TableField*>::iterator iter = table_field_map.find(fieldName);
	if(iter != table_field_map.end()){ 
		return (*iter).second->index;
	}
	return -1;
}


void SearchRocksDBIndex::GetRangeIndex(uint32_t range_type, InvertIndexEntry &startEntry, InvertIndexEntry &endEntry, std::vector<InvertIndexEntry>& resultEntry){
	log_debug("range_type:%d", range_type);
	
	if (range_type == RANGE_GELE) {
		GetRangeIndexGELE(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GE) {
		GetRangeIndexGE(startEntry, resultEntry);
	}
	else if (range_type == RANGE_LE) {
		GetRangeIndexLE(endEntry, resultEntry);
	}
	else if (range_type == RANGE_GTLT) {
		GetRangeIndexGTLT(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GTLE) {
		GetRangeIndexGTLE(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GELT) {
		GetRangeIndexGELT(startEntry, endEntry, resultEntry);
	}
	else if (range_type == RANGE_GT) {
		GetRangeIndexGT(startEntry, resultEntry);
	}
	else if (range_type == RANGE_LT) {
		GetRangeIndexLT(endEntry, resultEntry);
	}
}


void SearchRocksDBIndex::setEntry(DirectRequestContext &direct_request_context, std::vector<InvertIndexEntry>& entry){
	//获取信息
	std::vector<std::vector<string> > row_fields;
	uds_client->sendAndRecv(direct_request_context, row_fields, table_field_vec);
	log_debug("sendAndRecv is over");
	for(size_t i = 0; i < row_fields.size(); i++){
		int doc_id_index = getFieldIndex("doc_id");
		int doc_version_index = getFieldIndex("doc_version");
		int field_index = getFieldIndex("field");
		int key_index = getFieldIndex("key");

		InvertIndexEntry index_entry;
		index_entry._InvertIndexDocId = row_fields[i][doc_id_index];
		index_entry._InvertIndexDocVersion = atoi(row_fields[i][doc_version_index].c_str());
		index_entry._InvertIndexField = atoi(row_fields[i][field_index].c_str());
		// key的格式为10001#00#，需要先去掉这个前缀
		string strKey = row_fields[i][key_index];
		size_t pos = strKey.find_last_of("#", strKey.size() - 1);
		double key = 0;
		if(pos != string::npos){
			string subKey = strKey.substr(pos + 1);
			key = atof(subKey.c_str());
		}
		index_entry._InvertIndexKey = key;
		entry.push_back(index_entry);
	}
}


void SearchRocksDBIndex::setQueryCond(QueryCond& query_cond, int field_index, int cond_opr, int cond_val){
	query_cond.sFieldIndex = field_index;
	query_cond.sCondOpr = cond_opr;
	query_cond.sCondValue = cond_val;
}


void SearchRocksDBIndex::GetRangeIndexGELE(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGELE get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGELE get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;

	stringstream ss;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	
	o_range_query.sFieldConds.push_back(query_cond1);
	
	
	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGE;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL
		, begin_key._InvertIndex_key_type ,begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);


	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLE;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexGE(InvertIndexEntry& begin_key, std::vector<InvertIndexEntry>& entry){
	
	
	stringstream ss;

	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGE get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGE get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);


	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGE;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL, 
			begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLT;
	query_cond3.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, MAX_BORDER_SYMBOL, "");
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexLE(InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexLE get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexLE get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << end_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eLE;
	query_cond2.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eGT;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, MIN_BORDER_SYMBOL, "");
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexGTLT(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGTLT get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGTLT get key Index error");
		return;
	}
	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGT;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL
		, begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLT;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexGTLE(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGTLE get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGTLE get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGT;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL
		, begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLE;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexGELT(InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGELT get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGELT get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGE;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL
		, begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLT;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexGT(InvertIndexEntry& begin_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexGT get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexGT get key Index error");
		return;
	}
	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << begin_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eGT;
	query_cond2.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, INDEX_SYMBOL
		, begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eLT;
	query_cond3.sCondValue = gen_dtc_key_string(begin_key._InvertIndexAppid, MAX_BORDER_SYMBOL, "");
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetRangeIndexLT(InvertIndexEntry& end_key, std::vector<InvertIndexEntry>& entry){
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexLT get field Index error");
		return;
	}

	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexLT get key Index error");
		return;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << end_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);

	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = (uint8_t)CondOpr::eLT;
	query_cond2.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, INDEX_SYMBOL
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);

	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = (uint8_t)CondOpr::eGT;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, MIN_BORDER_SYMBOL, "");
	o_range_query.sFieldConds.push_back(query_cond3);

	DirectRequestContext direct_request_context;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

void SearchRocksDBIndex::GetScoreByCacheSet(const CacheQueryInfo &query_info, const set<string>& valid_docs, SkipList& skipList, map<string, uint32_t>& valid_version){

}

void SearchRocksDBIndex::GetScoreByCacheSetSearchAfter(const CacheQueryInfo &query_info, const set<string>& valid_docs, SkipList& skipList, map<string, uint32_t>& valid_version){

}

void SearchRocksDBIndex::GetRangeIndexInTerminal(RANGTYPE range_type, const InvertIndexEntry& begin_key, const InvertIndexEntry& end_key, const TerminalQryCond& query_cond, std::vector<InvertIndexEntry>& entry){
	
	if(getFieldIndex("field") == -1){
		log_error("GetRangeIndexInTerminal get field Index error");
		return;
	}
	if(getFieldIndex("key") == -1){
		log_error("GetRangeIndexInTerminal get key Index error");
		return;
	}

	bool sort_type = true;
	if(query_cond.sort_type == SORT_FIELD_DESC){
		sort_type = false;
	}

	int greater_type = (uint8_t)CondOpr::eGT;
	int less_type = (uint8_t)CondOpr::eLT;
	if(range_type == RANGE_GELE || range_type == RANGE_GELT || range_type == RANGE_GE){
		greater_type = (uint8_t)CondOpr::eGE;
	}
	if(range_type == RANGE_GELE || range_type == RANGE_GTLE || range_type == RANGE_LE){
		less_type = (uint8_t)CondOpr::eLE;
	}
	string begin_symbol = INDEX_SYMBOL;
	string end_symbol = INDEX_SYMBOL;
	if(range_type == RANGE_GE || range_type == RANGE_GT){
		end_symbol = MAX_BORDER_SYMBOL;
	}
	if(range_type == RANGE_LE || range_type == RANGE_LT){
		begin_symbol = MIN_BORDER_SYMBOL;
	}

	RangeQuery_t o_range_query;

	QueryCond query_cond1;
	query_cond1.sFieldIndex = getFieldIndex("field");
	query_cond1.sCondOpr = (uint8_t)CondOpr::eEQ;
	stringstream ss;
	ss << end_key._InvertIndexField;
	query_cond1.sCondValue = ss.str();
	o_range_query.sFieldConds.push_back(query_cond1);


	QueryCond query_cond2;
	query_cond2.sFieldIndex = getFieldIndex("key");
	query_cond2.sCondOpr = greater_type;
	query_cond2.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, begin_symbol
		, begin_key._InvertIndex_key_type , begin_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond2);


	QueryCond query_cond3;
	query_cond3.sFieldIndex = getFieldIndex("key");
	query_cond3.sCondOpr = less_type;
	query_cond3.sCondValue = gen_dtc_key_string(end_key._InvertIndexAppid, end_symbol
		, end_key._InvertIndex_key_type , end_key._InvertIndexKey);
	o_range_query.sFieldConds.push_back(query_cond3);

	// key和docd_id对应的field值分别为0和1
	std::pair<int, bool> order_cond1;
	order_cond1 = make_pair(0, sort_type);
	o_range_query.sOrderbyFields.push_back(order_cond1);
	std::pair<int, bool> order_cond2;
	order_cond2 = make_pair(1, sort_type);
	o_range_query.sOrderbyFields.push_back(order_cond2);

	LimitCond sLimitCond;
	sLimitCond.sLimitStart = query_cond.limit_start;
	sLimitCond.sLimitStep = query_cond.page_size;
	o_range_query.sLimitCond = sLimitCond;

	DirectRequestContext direct_request_context;
	direct_request_context.sMagicNum = 12345;
	direct_request_context.sSequenceId = 10;
	direct_request_context.sDirectQueryType = (uint8_t)DirectRequestType::eRangeQuery;
	direct_request_context.sPacketValue.uRangeQuery = (uint64_t)(&o_range_query);

	setEntry(direct_request_context, entry);
}

bool SearchRocksDBIndex::InsertIndex(const InvertIndexEntry& entry){
	return true;
}

bool SearchRocksDBIndex::DeleteIndex(const InvertIndexEntry& entry){
	return true;
}

void SearchRocksDBIndex::DumpIndex(){

}
