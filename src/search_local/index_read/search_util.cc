﻿/*
 * =====================================================================================
 *
 *       Filename:  search_util.h
 *
 *    Description:  search util class definition.
 *
 *        Version:  1.0
 *        Created:  22/03/2018
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhulin, shzhulin3@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */

#include <algorithm>
#include "log.h"
#include "search_util.h"
#include "stdlib.h"
#include "data_manager.h"
#include "split_manager.h"
#include "geohash.h"
#include <sstream>
#include <iterator>
#include <set>
#include <string>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <string.h>
#include "utils/get_aois_action.h"
#include "process/geo_distance_query_process.h"
using namespace std;

string initial_table[] = { "b","p","m","f","d","t","n","l","g","k","h","j","q","x","zh","ch","sh","r","z","c","s","y","w" };

typedef struct {
    char syllable[8];
    char small_syllable[2][8];
}SMALLSYLLABLEITEM;

static SMALLSYLLABLEITEM small_syllable_items[] =
{
    {"ao", {"a", "o"}},
    {"bian", {"bi", "an"}},
    {"yue", {"yu", "e"}},
    {"biao", {"bi", "ao"}},
    {"cuan", {"cu", "an"}},
    {"dia", {"di", "a"}},
    {"dian", {"di", "an"}},
    {"diao", {"di", "ao"}},
    {"die", {"di", "e"}},
    {"duan", {"du", "an"}},
    {"gua", {"gu", "a"}},
    {"guai", {"gu", "ai"}},
    {"guan", {"gu", "an"}},
    {"hua", {"hu", "a"}},
    {"huai", {"hu", "ai"}},
    {"huan", {"hu", "an"}},
    {"huo", {"hu", "o"}},
    {"jia", {"ji", "a"}},
    {"jian", {"ji", "an"}},
    {"jiang", {"ji", "ang"}},
    {"jiao", {"ji", "ao"}},
    {"jie", {"ji", "e"}},
    {"jue", {"ju", "e"}},
    {"juan", {"ju", "an"}},
    {"kua", {"ku", "a"}},
    {"kuai", {"ku", "ai"}},
    {"kuo", {"ku", "o"}},
    {"lia", {"li", "a"}},
    {"lian", {"li", "an"}},
    {"liang", {"li", "ang"}},
    {"liao", {"li", "ao"}},
    {"lie", {"li", "e"}},
    {"luan", {"lu", "an"}},
    {"mian", {"mi", "an"}},
    {"miao", {"mi", "ao"}},
    {"mie", {"mi", "e"}},
    {"nao", {"na", "o"}},
    {"nian", {"ni", "an"}},
    {"niao", {"ni", "ao"}},
    {"nie", {"ni", "e"}},
    {"pian", {"pi", "an"}},
    {"piao", {"pi", "ao"}},
    {"pie", {"pi", "e"}},
    {"qia", {"qi", "a"}},
    {"qian", {"qi", "an"}},
    {"qiang", {"qi", "ang"}},
    {"qiao", {"qi", "ao"}},
    {"qie", {"qi", "e"}},
    {"quan", {"qu", "an"}},
    {"que", {"qu", "e"}},
    {"suan", {"su", "an"}},
    {"shuan", {"shu", "an"}},
    {"shuang", {"shu", "ang"}},
    {"shuo", {"shu", "o"}},
    {"tian", {"ti", "an"}},
    {"tie", {"ti", "e"}},
    {"tuan", {"tu", "an"}},
    {"tuo", {"tu", "o"}},
    {"xia", {"xi", "a"}},
    {"xian", {"xi", "an"}},
    {"xiao", {"xi", "ao"}},
    {"xiang", {"xi", "ang"}},
    {"xie", {"xi", "e"}},
    {"xuan", {"xu", "an"}},
    {"xue", {"xu", "e"}},
    {"yao", {"ya", "o"}},
    {"yuan", {"yu", "an"}},
    {"yue", {"yu", "e"}},
    {"zao", {"za", "o"}},
    {"zuan", {"zu", "an"}},
    {"zhao", {"zha", "o"}},
    {"zhua", {"zhu", "a"}},
    {"zhuan", {"zhu", "an"}}
};

bool IsSmallSyllable(string str, string &syllable1, string &syllable2) {
    int num = sizeof(small_syllable_items) / sizeof(SMALLSYLLABLEITEM);
    for (int i = 0; i < num; i++) {
        if (strcmp(small_syllable_items[i].syllable, str.c_str()) == 0) {
            syllable1 = (small_syllable_items[i].small_syllable)[0];
            syllable2 = (small_syllable_items[i].small_syllable)[1];
            return true;
        }
    }
    return false;
}

bool operator<(const Content &a, const Content &b) {
    if (a.type > b.type) {
        return false;
    }
    else {
        return a.str < b.str;
    }
}

string readFileIntoString(char * filename)
{
    ifstream ifile(filename);
    //将文件读入到ostringstream对象buf中
    ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
        buf.put(ch);
    //返回与流对象buf关联的字符串
    return buf.str();
}

vector<int> splitInt(const string& src, string separate_character)
{
    vector<int> strs;

    //分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
    int separate_characterLen = separate_character.size();
    int lastPosition = 0, index = -1;
    string str;
    int pos = 0;
    while (-1 != (index = src.find(separate_character, lastPosition)))
    {
        if (src.substr(lastPosition, index - lastPosition) != " ") {
            str = src.substr(lastPosition, index - lastPosition);
            pos = atoi(str.c_str());
            strs.push_back(pos);
        }
        lastPosition = index + separate_characterLen;
    }
    string lastString = src.substr(lastPosition);//截取最后一个分隔符后的内容
    if (!lastString.empty() && lastString != " ")
        pos = atoi(lastString.c_str());
        strs.push_back(pos);//如果最后一个分隔符后还有内容就入队
    return strs;
}

set<string> splitStr(const string& src, string separate_character)
{
    set<string> strs;

    //分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
    int separate_characterLen = separate_character.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = src.find(separate_character, lastPosition)))
    {
        if (src.substr(lastPosition, index - lastPosition) != " ") {
            strs.insert(src.substr(lastPosition, index - lastPosition));
        }
        lastPosition = index + separate_characterLen;
    }
    string lastString = src.substr(lastPosition);//截取最后一个分隔符后的内容
    if (!lastString.empty() && lastString != " ")
        strs.insert(lastString);//如果最后一个分隔符后还有内容就入队
    return strs;
}

bool CheckWordContinus(const vector<string> &word_vec, map<string, vector<int> > &pos_map) {
    if (word_vec.size() < 2) {
        return true;
    }

    size_t i = 0;
    string word_before = "";
    string word_after = "";
    bool all_match = true;
    for (; i < word_vec.size() - 1; i++) {
        word_before = word_vec[i];
        word_after = word_vec[i + 1];
        vector<int> before_vec;
        vector<int> after_vec;
        if (pos_map.find(word_before) != pos_map.end()) {
            before_vec = pos_map[word_before];
        }
        else {
            all_match = false;
            break;
        }
        if (pos_map.find(word_after) != pos_map.end()) {
            after_vec = pos_map[word_after];
        }
        else {
            all_match = false;
            break;
        }

        size_t j = 0;
        size_t k = 0;
        int before_pos = 0;
        int after_pos = 0;
        bool match_flag = false;
        for (j = 0; j < before_vec.size(); j++) {
            if (match_flag == true) {
                break;
            }
            before_pos = before_vec[j];
            for (k = 0; k < after_vec.size(); k++) {
                after_pos = after_vec[k];
                if (after_pos - before_pos <= 2) {
                    match_flag = true;
                    break;
                }
            }
        }

        if (match_flag == false) {
            all_match = false;
            break;
        }
    }

    if (all_match == true) {
        return true;
    }
    else {
        return false;
    }
}

void split_func(string pinyin, string &split_str, int type) {
    int i = 0;
    stringstream result;
    for (i = 0; i<(int)pinyin.size(); i++)
    {
        if (strchr("aeiouv", pinyin.at(i)))
        {
            result << pinyin.at(i);
            continue;
        }
        else
        {
            if (pinyin.at(i) != 'n')  //不是n从该辅音前分开
            {
                if (i == 0)
                {
                    result << pinyin.at(i);
                }
                else
                {
                    result << ' ' << pinyin.at(i);
                }
                if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
                    (pinyin.at(i + 1) == 'h'))
                {
                    if (type == 1) {
                        result << 'h';
                    }
                    else {
                        result << " h";
                    }
                    i++;
                }
                continue;
            }
            else                 //是n,继续向后
            {
                if (i == (int)pinyin.size() - 1)
                {
                    result << pinyin.at(i);
                    continue;
                }
                else
                    i++;   //继续向后

                if (strchr("aeiouv", pinyin.at(i)))   //如果是元音,从n前分开
                {
                    if (i == 1)
                    {
                        result << 'n' << pinyin.at(i);
                        continue;
                    }
                    else
                    {
                        result << ' ' << 'n' << pinyin.at(i);
                        continue;
                    }
                }
                //如果是辅音字母
                else
                {
                    if (pinyin.at(i) == 'g')
                    {
                        if (i == (int)pinyin.size() - 1)
                        {
                            result << 'n' << pinyin.at(i);
                            continue;
                        }
                        else
                            i++;  //继续向后

                        if (strchr("aeiouv", pinyin.at(i)))
                        {
                            result << 'n' << ' ' << 'g' << pinyin.at(i);
                            continue;
                        }
                        else
                        {
                            result << 'n' << 'g' << ' ' << pinyin.at(i);
                            if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
                                (pinyin.at(i + 1) == 'h'))
                            {
                                if (type == 1) {
                                    result << 'h';
                                }
                                else {
                                    result <<" h";
                                }
                                i++;
                            }
                            continue;
                        }
                    }
                    else   //不是g的辅音字母,从n后分开
                    {
                        result << 'n' << ' ' << pinyin.at(i);
                        if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
                            (pinyin.at(i + 1) == 'h'))
                        {
                            if (type == 1) {
                                result << 'h';
                            }
                            else {
                                result << " h";
                            }
                            i++;
                        }
                        continue;
                    }
                }
            }
        }
    }
    split_str = result.str();
}

void FMM(string &str, vector<string> &vec) {
    int maxLen = 6;
    int len_phrase = str.length();
    int i = 0, j = 0;

    while (i < len_phrase) {
        int end = i + maxLen;
        if (end >= len_phrase) {
            end = len_phrase;
        }
        string sub = str.substr(i, end - i);
        for (j = sub.length(); j >= 0; j--) {
            if (j == 1)
                break;
            string key = sub.substr(0, j);
            if (DataManager::Instance()->IsPhonetic(key) == true) {
                vec.push_back(key);
                i += key.length() - 1;
                break;
            }
        }
        if (j == 1) {
            vec.push_back(string(1, sub[0]));
        }
        i += 1;
    }
    return;
}

set<uint32_t> sets_intersection(set<uint32_t> v1, set<uint32_t> v2) {
    set<uint32_t> v;
    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), inserter(v, v.begin()));//求交集 
    return v;
}

set<string> sets_intersection(set<string> v1, set<string> v2) {
    set<string> v;
    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), inserter(v, v.begin()));//求交集 
    return v;
}

set<string> sets_union(set<string> v1, set<string> v2) {
    set<string> v;
    set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), inserter(v, v.begin()));//求并集 
    return v;
}

set<string> sets_difference(set<string> v1, set<string> v2) {
    set<string> v;
    set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), inserter(v, v.begin()));//求差集 
    return v;
}

int GetMultipleWords(string m_Data, set<vector<Content> >& cvset) {
    vector<Content> result;
    set<string> initial_vec(initial_table, initial_table + 23);
    iutf8string utf8_str(m_Data);
    int i = 0;
    bool isAllChinese = true;
    for (; i < utf8_str.length(); ) {
        if (utf8_str[i].size() > 1) {
            Content content;
            content.type = CHINESE;
            content.str = utf8_str[i];
            result.push_back(content);
            i++;
        }
        else {
            isAllChinese = false;
            string tmp = utf8_str[i];
            int j = 1;
            for (; j < utf8_str.length() - i; j++) {
                if (utf8_str[i + j].size() > 1) {
                    break;
                }
                tmp += utf8_str[i + j];
            }
            i += j;
            string split_str;
            split_func(tmp, split_str);
            vector<string> vec = splitEx(split_str, " ");
            vector<string>::iterator iter = vec.begin();
            for (; iter != vec.end(); iter++) {
                Content content;
                content.str = *iter;
                if (initial_vec.find(*iter) != initial_vec.end()) {
                    content.type = INITIAL;
                }
                else if ((*iter).size() == 1) {
                    content.type = INITIAL;
                }
                else {
                    content.type = WHOLE_SPELL;
                }
                result.push_back(content);
            }
        }
    }
    cvset.insert(result);
    if (isAllChinese == false) {
        // 歧义拼音继续拆分 yue --> yu e
        vector<Content> syllableVec;
        vector<Content>::iterator iter = result.begin();
        for (; iter != result.end(); iter++) {
            Content content = *iter;
            string syllable1;
            string syllable2;
            if (content.type == WHOLE_SPELL) {
                bool flag = IsSmallSyllable(content.str, syllable1, syllable2);
                if (flag == true) {
                    Content content1;
                    Content content2;
                    content1.str = syllable1;
                    content1.type = WHOLE_SPELL;
                    content2.str = syllable2;
                    content2.type = WHOLE_SPELL;
                    syllableVec.push_back(content1);
                    syllableVec.push_back(content2);
                }
                else {
                    syllableVec.push_back(content);
                }
            }
            else {
                syllableVec.push_back(content);
            }
        }
        cvset.insert(syllableVec);

        // 将zh ch sh切分开
        vector<Content> cvec;
        for (i = 0; i < utf8_str.length(); ) {
            if (utf8_str[i].size() > 1) {
                Content content;
                content.type = CHINESE;
                content.str = utf8_str[i];
                cvec.push_back(content);
                i++;
            }
            else {
                isAllChinese = false;
                string tmp = utf8_str[i];
                int j = 1;
                for (; j < utf8_str.length() - i; j++) {
                    if (utf8_str[i + j].size() > 1) {
                        break;
                    }
                    tmp += utf8_str[i + j];
                }
                i += j;
                string split_str;
                split_func(tmp, split_str, 2);
                vector<string> vec = splitEx(split_str, " ");
                vector<string>::iterator iter = vec.begin();
                for (; iter != vec.end(); iter++) {
                    Content content;
                    content.str = *iter;
                    if (initial_vec.find(*iter) != initial_vec.end()) {
                        content.type = INITIAL;
                    }
                    else if ((*iter).size() == 1) {
                        content.type = INITIAL;
                    }
                    else {
                        content.type = WHOLE_SPELL;
                    }
                    cvec.push_back(content);
                }
            }
        }
        cvset.insert(cvec);

        // 最大匹配进行拼音拆分，对于wuenda这种可以有效支持
        vector<Content> fmmVec;
        for (i = 0; i < utf8_str.length(); ) {
            if (utf8_str[i].size() > 1) {
                Content content;
                content.type = CHINESE;
                content.str = utf8_str[i];
                fmmVec.push_back(content);
                i++;
            }
            else {
                isAllChinese = false;
                string tmp = utf8_str[i];
                int j = 1;
                for (; j < utf8_str.length() - i; j++) {
                    if (utf8_str[i + j].size() > 1) {
                        break;
                    }
                    tmp += utf8_str[i + j];
                }
                i += j;
                vector<string> vec;
                FMM(tmp, vec);
                vector<string>::iterator iter = vec.begin();
                for (; iter != vec.end(); iter++) {
                    Content content;
                    content.str = *iter;
                    if (initial_vec.find(*iter) != initial_vec.end()) {
                        content.type = INITIAL;
                    }
                    else if ((*iter).size() == 1) {
                        content.type = INITIAL;
                    }
                    else {
                        content.type = WHOLE_SPELL;
                    }
                    fmmVec.push_back(content);
                }
            }
        }
        cvset.insert(fmmVec);
    }

    return 0;
}

void ConvertCharIntelligent(const string word, IntelligentInfo &info, int &len) {
    int i = 0;
    int index = 0;
    int length = word.size();

    for (; index < length; index++, i++) {
        if (index > 15){
            break;
        }
        info.initial_char[i] = word[index];
    }
    len = index;
    return ;
}

void ConvertIntelligent(const vector<Content> &result, IntelligentInfo &info, bool &flag) {
    int i = 0;
    flag = true;
    vector<Content>::const_iterator content_iter = result.begin();
    for (; content_iter != result.end(); content_iter++, i++) {
        if (i > 7)
            break;

        uint32_t charact_id = 0;
        uint32_t phonetic_id = 0;

        if ((*content_iter).type == CHINESE) { // 查找字id
            DataManager::Instance()->GetCharactId((*content_iter).str, charact_id);
            info.charact_id[i] = charact_id;
        }
        else if ((*content_iter).type == WHOLE_SPELL) { // 查找拼音id
			DataManager::Instance()->GetPhoneticId((*content_iter).str, phonetic_id);
			info.phonetic_id[i] = phonetic_id;
		}
		else {
			info.initial_char[i] = (*content_iter).str;
		}

        // 如果查找id为0，则视为无效
        if (charact_id == 0 && phonetic_id == 0 && (*content_iter).type != INITIAL) {
            flag = false;
            break;
        }
    }
}

vector<Content> GetSingleWord(string m_Data)
{
    vector<Content> result;
    set<string> initial_vec(initial_table, initial_table + 23);
    iutf8string utf8_str(m_Data);
    int i = 0;
    for (; i < utf8_str.length(); ) {
        if (utf8_str[i].size() > 1) {
            Content content;
            content.type = CHINESE;
            content.str = utf8_str[i];
            result.push_back(content);
            i++;
        }
        else {
            string tmp = utf8_str[i];
            int j = 1;
            for (; j < utf8_str.length() - i; j++) {
                if (utf8_str[i + j].size() > 1) {
                    break;
                }
                tmp += utf8_str[i + j];
            }
            i += j;
            string split_str;
            split_func(tmp, split_str);
            vector<string> vec = splitEx(split_str, " ");
            vector<string>::iterator iter = vec.begin();
            for (; iter != vec.end(); iter++) {
                Content content;
                content.str = *iter;
                if (initial_vec.find(*iter) != initial_vec.end()) {
                    content.type = INITIAL;
                } else if ((*iter).size() == 1) {
                    content.type = INITIAL;
                }
                else {
                    content.type = WHOLE_SPELL;
                }
                result.push_back(content);
            }
        }
    }

    return result;
}

int GetSuggestWord(string m_Data, vector<string> &word_vec, uint32_t suggest_cnt) {
    vector<Content> result = GetSingleWord(m_Data);
    IntelligentInfo info;
    bool flag = true;
    ConvertIntelligent(result, info, flag);
    if (flag == false) {
        log_debug("ConvertIntelligent invalid.");
        return 0;
    }
    if (info.initial_char[0] == "" && info.charact_id[0] == 0) {
        if (m_Data.length() != 1) {
            uint32_t charact_id = 0;
            DataManager::Instance()->GetCharactId(m_Data, charact_id);
            info.charact_id[0] = charact_id;
        }
        else {
            info.initial_char[0] = m_Data;
        }
    }
    log_debug("charact_id_01: %d, charact_id_02: %d,charact_id_03: %d, charact_id_04: %d, phonetic_id_01: %d, phonetic_id_02: %d, \
        phonetic_id_03: %d, phonetic_id_04: %d, initial_char_01: %s, initial_char_02: %s, initial_char_03: %s, initial_char_04: %s",
        info.charact_id[0], info.charact_id[1], info.charact_id[2], info.charact_id[3], info.phonetic_id[0], info.phonetic_id[1],
        info.phonetic_id[2], info.phonetic_id[3], info.initial_char[0].c_str(), info.initial_char[1].c_str(), info.initial_char[2].c_str(), 
        info.initial_char[3].c_str());

    if (DataManager::Instance()->GetSuggestWord(info, word_vec, suggest_cnt) == false) {
        return -RT_GET_SUGGEST_ERR;
    }

    return 0;
}

int GetEnSuggestWord(string m_Data, vector<string> &word_vec, uint32_t suggest_cnt) {
    vector<Content> result;
    size_t i = 0;
    for (; i < m_Data.length(); i++) {
        Content content;
        content.str = m_Data[i];
        content.type = INITIAL;
        result.push_back(content);
    }

    IntelligentInfo info;
    bool flag = true;
    ConvertIntelligent(result, info, flag);
    if (flag == false) {
        log_debug("ConvertIntelligent invalid.");
        return 0;
    }
    log_debug("initial_char_01: %s, initial_char_02: %s, initial_char_03: %s, initial_char_04: %s",
        info.initial_char[0].c_str(), info.initial_char[1].c_str(), info.initial_char[2].c_str(), info.initial_char[3].c_str());

    if (DataManager::Instance()->GetEnSuggestWord(info, word_vec, suggest_cnt) == false) {
        return -RT_GET_SUGGEST_ERR;
    }

    return 0;
}

bool isAllNumber(string str) {
    bool flag = true;
    size_t i = 0;
    for (; i < str.size(); i++) {
        if ((str[i] < '0') || (str[i] > '9')) {
            flag = false;
            break;
        }
    }
    return flag;
}

bool isAllAlpha(string str) {
    bool flag = true;
    size_t i = 0;
    for (; i < str.size(); i++) {
        if (!isupper(str[i]) && !islower(str[i])) {
            flag = false;
            break;
        }
    }
    return flag;
}

bool isContainCharacter(string str) {
    bool flag = false;
    size_t i = 0;
    for (; i < str.size(); i++) {
        if (str[i] < 0) {
            flag = true;
            break;
        }
    }
    return flag;
}

bool isAllChinese(string str) {
    bool flag = true;
    iutf8string utf8_str(str);
    int i = 0;
    for (; i < utf8_str.length(); i++) {
        if (utf8_str[i].size() == 1) { // 如果有长度为1的情况，则不是全汉字
            flag = false;
            break;
        }
    }

    return flag;
}

int judgeDataType(string str) {
    int data_type = 0;
    if (isAllAlpha(str)) {
        if (DataManager::Instance()->IsEnWord(str)) {
            data_type = DATA_ENGLISH;
        }
        else {
            data_type = DATA_PHONETIC;
        }
    }
    else {
        if (isAllChinese(str)) {
            data_type = DATA_CHINESE;
        }
        else {
            data_type = DATA_HYBRID;
        }
    }
    return data_type;
}

int Convert2Phonetic(string str, string &phonetic) {
    phonetic = "";
    iutf8string utf8_str(str);
    int i = 0;
    for (; i < utf8_str.length(); i++) {
        phonetic.append(DataManager::Instance()->GetPhonetic(utf8_str[i]));
    }
    return 0;
}

int GetInitialVec(IntelligentInfo &info, int len)
{
    int count = 0;
    vector<string> initials(8, "");

    for(int i = 0; i < 8; i++){
        if (info.initial_char[i] != "")
            initials[count++] = info.initial_char[i];
    }

    if (count == 0 || count == 8)
        return -1;

    for (int i = count - 1; i >= 0; i--)
    {
        if (i + len >= 8)
            return -1;
        initials[i + len] = initials[i];
    }

    for (int i = 0; i < len; i++)
    {
        initials[i] = "";
    }

    for(int i = 0; i < 8; i++){
        info.initial_char[i] = initials[i];
    }

    return 0;
}

int ShiftIntelligentInfoWithoutCharacter(IntelligentInfo &info, int len)
{
    int count = 0;
    vector<string> initials(16, "");

    for(int i = 0; i < 16; i++){
        if (info.initial_char[i] != "")
            initials[count++] = info.initial_char[i];
    }

    if (count <= 0 || count >= 16)
        return -1;

    for (int i = count - 1; i >= 0; i--)
    {
        if (i + len >= 16)
            return -1;
        initials[i + len] = initials[i];
    }

    for (int i = 0; i < len; i++)
    {
        initials[i] = "";
    }

    for(int i = 0; i < 16; i++){
        info.initial_char[i] = initials[i];
    }

    return 0;
}

const char *GetFormatTimeStr(uint32_t ulTime)
{
    static char acTimeStr[64];
    struct  tm stTm;

    time_t myTime = ulTime;//time_t 在64位是8字节

    memset(acTimeStr, 0, sizeof(acTimeStr));

    localtime_r(&myTime, &stTm);

    snprintf(acTimeStr, sizeof(acTimeStr) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
        stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

    return acTimeStr;
}

void ToLower(string &str) {
    size_t i = 0;
    for (; i < str.size(); i++) {
        if (str[i]>='A' && str[i]<='Z') {
            str[i] = str[i]+32;
        }
    }
}

string CharToString(char c) 
{
    stringstream stream;
    stream << c;
    return stream.str();
}

string ToString(uint32_t appid)
{
    ostringstream oss;
    oss<<appid;
    return oss.str();
}

bool GetGisCode(string lng, string lat, string ip, double distance, vector<string>& gisCode)
{
    if ((lng != "") && (lat != "")) {
        GeoPoint geo1;
        geo1.lon = atof(lng.c_str());
        geo1.lat = atof(lat.c_str());
        gisCode = GetArroundGeoHash(geo1, distance, 6);
    } else if(ip != ""){
        return true;
    } else {
        return false;
    }
    return true;
}

bool GetGisCode(const vector<string>& lng_arr, const vector<string>& lat_arr, vector<string>& gisCode){
    double lng_max = atof(lng_arr[0].c_str());
    double lng_min = atof(lng_arr[0].c_str());
    double lat_max = atof(lat_arr[0].c_str());
    double lat_min = atof(lat_arr[0].c_str());
    for(size_t i = 1; i < lng_arr.size(); i++){
        if(lng_max < atof(lng_arr[i].c_str())){
            lng_max = atof(lng_arr[i].c_str());
        }
        if(lng_min > atof(lng_arr[i].c_str())){
            lng_min = atof(lng_arr[i].c_str());
        }
    }
    for(size_t i = 1; i < lat_arr.size(); i++){
        if(lat_max < atof(lat_arr[i].c_str())){
            lat_max = atof(lat_arr[i].c_str());
        }
        if(lat_min > atof(lat_arr[i].c_str())){
            lat_min = atof(lat_arr[i].c_str());
        }
    }
    EnclosingRectangle enclose_rectangle(lng_max , lng_min , lat_max , lat_min);

    gisCode = GetArroundGeoHash(enclose_rectangle, 6);
    return true;
}

double strToDouble(const string& str)
{
    double num;
    istringstream iss(str);
    iss >> num;
    return num;
}

double toRadians(double d){
    return d * 3.1415 / 180.0;
}

double distanceSimplify(double lat1, double lng1, double lat2, double lng2) {
    double dx = lng1 - lng2; // 经度差值
    double dy = lat1 - lat2; // 纬度差值
    double b = (lat1 + lat2) / 2.0; // 平均纬度
    double Lx = toRadians(dx) * 6378.137 * cos(toRadians(b)); // 东西距离
    double Ly = 6378.137 * toRadians(dy); // 南北距离
    return sqrt(Lx * Lx + Ly * Ly);  // 用平面的矩形对角距离公式计算总距离
}

bool GetGisDistance(uint32_t appid, const GeoPointContext& geo_point, const hash_string_map& doc_content, hash_double_map& distances)
{
    hash_string_map::const_iterator doc_it;
    for (doc_it = doc_content.cbegin() ; doc_it != doc_content.cend(); doc_it++) {
        if (doc_it->second == "") {
            log_error("content is invalid, appid:%d, doc_id:%s, content:%s.",appid, (doc_it->first).c_str(), (doc_it->second).c_str());
            continue;
        }

        Json::Reader read(Json::Features::strictMode());
        Json::Value snap_json;
        int ret = read.parse(doc_it->second , snap_json);
        if (0 == ret){
            log_error("parse json error [%s], errmsg : %s", doc_it->second.c_str(), read.getFormattedErrorMessages().c_str());
            return false;
        }
        
        Json::Value::Members member = snap_json.getMemberNames();
        Json::Value::Members::iterator iter = member.begin();
        for (; iter != member.end(); ++iter){
            uint32_t segment_tag = 0;
            FieldInfo field_info;
            uint32_t uiret = DBManager::Instance()->GetWordField(segment_tag, appid, *iter, field_info);
            if (geo_point.IsGeoPointFormat()){
                if (uiret != 0 && FIELD_GEO_POINT == field_info.field_type){
                GeoPointContext target_geo_point(snap_json[*iter]);
                double d_query_lat = strToDouble(geo_point.sLatitude);
                double d_query_lng = strToDouble(geo_point.sLongtitude);
                double d_target_lat = strToDouble(target_geo_point.sLatitude);
                double d_target_lng = strToDouble(target_geo_point.sLongtitude);
                double dis = round(distanceSimplify(d_query_lat, d_query_lng,
                                                    d_target_lat, d_target_lng)* 1000) / 1000;

                    if ((geo_point.d_distance > -0.0001 && geo_point.d_distance < 0.0001) 
                            || (dis + 1e-6 <= geo_point.d_distance)){
                        distances[doc_it->first] = dis;
                        log_debug("query geopoint lat:%f , lng:%f , query target geopoint lat:%f , lng:%f , dis:%f"  \
                            , d_query_lat , d_query_lng , d_target_lat , d_target_lng , dis);
                    }
                }
            }else if (uiret != 0 && FIELD_GEO_SHAPE == field_info.field_type){
                // temp no handle ,latter add
                distances[doc_it->first] = 1;
                log_debug("query target geoshape handle here");
            }
        }
    }
    return true;
}

uint32_t GetIpNum(string ip)
{
    uint32_t s;
    if (inet_pton(AF_INET, ip.c_str(), (void *)&s) == 0){
        log_error("resolve ip error. ip : %s.", ip.c_str());
        return 0;
    }
    return ntohl(s);
}

uint64_t GetSysTimeMicros(){
    timeval tv;
    memset(&tv, 0, sizeof(tv));

    if (0 != gettimeofday(&tv, NULL)){
        return 0;
    }

    uint64_t ullTime = (uint64_t)tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
    return ullTime;
}

string trim(string& str)
{
    str.erase(0, str.find_first_not_of(" ")); // 去掉头部空格
    str.erase(str.find_last_not_of(" ") + 1); // 去掉尾部空格
    return str;
}

string delPrefix(string& str){
    size_t pos1 = str.find_first_of("((");
    size_t pos2 = str.find_last_of("))");
    string res = str;
    if(pos1 != string::npos && pos2 != string::npos){
        res = str.substr(pos1+2, pos2-pos1-3);
    }
    return res;
}