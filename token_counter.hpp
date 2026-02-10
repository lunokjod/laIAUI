#ifndef TOKEN_COUNTER_HPP
#define TOKEN_COUNTER_HPP

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class TokenCounter {
private:
    static unordered_map<string, int> getBpeRanks();
    static vector<pair<string, int>> getByteEncoder();
    
public:
    static int countTokens(const string& text);
    static int countChatMessageTokens(const string& role, const string& content);
    static int estimateConversationTokens(const vector<pair<string, string>>& messages);
};

#endif