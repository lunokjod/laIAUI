#include "token_counter.hpp"
#include <cctype>
#include <algorithm>

unordered_map<string, int> TokenCounter::getBpeRanks() {
    unordered_map<string, int> ranks;
    int rank = 0;
    
    // Caràcters individuals (ASCII)
    for (int i = 0; i < 256; i++) {
        string ch(1, static_cast<char>(i));
        ranks[ch] = rank++;
    }
    
    // Patrons comuns en anglès/català
    vector<string> commonPatterns = {
        " the", " a", " an", " of", " to", " in", " for", " on", " with",
        " is", " are", " was", " were", " be", " been", " have", " has",
        " had", " do", " does", " did", " not", " and", " or", " but",
        " if", " then", " else", " when", " where", " why", " how",
        " all", " any", " both", " each", " few", " more", " most",
        " other", " some", " such", " no", " nor", " not", " only",
        " own", " same", " so", " than", " too", " very", " can",
        " will", " just", " don", " should", " now", " el", " la",
        " els", " les", " un", " una", " uns", " unes", " de", " i",
        " que", " en", " és", " no", " per", " amb", " com", " del",
        " al", " als", " de la", " de les", " a la", " a les"
    };
    
    for (const auto& pattern : commonPatterns) {
        ranks[pattern] = rank++;
    }
    
    return ranks;
}

vector<pair<string, int>> TokenCounter::getByteEncoder() {
    vector<pair<string, int>> encoder;
    
    // Caràcters ASCII
    for (int i = 0; i < 256; i++) {
        encoder.push_back({string(1, static_cast<char>(i)), i});
    }
    
    return encoder;
}

int TokenCounter::countTokens(const string& text) {
    if (text.empty()) return 0;
    
    int charCount = text.length();
    int wordCount = 0;
    int spaceCount = 0;
    int punctuationCount = 0;
    int digitCount = 0;
    
    bool inWord = false;
    for (char c : text) {
        if (isspace(c)) {
            spaceCount++;
            if (inWord) {
                wordCount++;
                inWord = false;
            }
        } else if (ispunct(c)) {
            punctuationCount++;
            if (inWord) {
                wordCount++;
                inWord = false;
            }
        } else if (isdigit(c)) {
            digitCount++;
            inWord = true;
        } else {
            inWord = true;
        }
    }
    
    if (inWord) {
        wordCount++;
    }
    
    double estimatedTokens = 0;
    estimatedTokens += wordCount * 1.0;
    estimatedTokens += (punctuationCount + spaceCount) * 0.3;
    estimatedTokens += digitCount * 0.5;
    
    int otherChars = charCount - wordCount - punctuationCount - spaceCount - digitCount;
    estimatedTokens += otherChars * 0.25;
    
    double languageFactor = 1.1;
    
    bool hasCode = text.find("```") != string::npos || 
                  text.find("#include") != string::npos ||
                  text.find("def ") != string::npos ||
                  text.find("function") != string::npos ||
                  text.find("class ") != string::npos;
    
    if (hasCode) {
        languageFactor = 1.3;
    }
    
    estimatedTokens *= languageFactor;
    
    if (estimatedTokens < 1 && charCount > 0) {
        estimatedTokens = 1;
    }
    
    return static_cast<int>(estimatedTokens + 0.5);
}

int TokenCounter::countChatMessageTokens(const string& role, const string& content) {
    int totalTokens = 0;
    totalTokens += 10;
    totalTokens += countTokens(role) + 2;
    totalTokens += countTokens(content);
    return totalTokens;
}

int TokenCounter::estimateConversationTokens(const vector<pair<string, string>>& messages) {
    int totalTokens = 0;
    totalTokens += 50;
    
    for (const auto& [role, content] : messages) {
        totalTokens += countChatMessageTokens(role, content);
    }
    
    totalTokens += 100;
    return totalTokens;
}