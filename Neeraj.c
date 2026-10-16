struct Result {
    int conditionIndex;
    double score;
    int matchedCount;
};

vector<Result> rankConditions(const vector<Condition>& conditions,
                              const vector<string>& symptoms) {
    vector<Result> results;
    for (int i = 0; i < (int)conditions.size(); i++) {
        int matchedWeight = 0, matched = 0;
        for (auto& s : symptoms) {
            int w = conditions[i].weightOf(s);
            if (w > 0) { matchedWeight += w; matched++; }
        }
        if (matched == 0) continue;
        double coverage = (double)matchedWeight / conditions[i].getTotalWeight();
        double explained = (double)matched / symptoms.size();
        results.push_back({i, 0.7 * coverage + 0.3 * explained, matched});
    }
    if (results.empty()) throw NoMatchException();
    sort(results.begin(), results.end(),
         [](const Result& a, const Result& b) { return a.score > b.score; });
    return results;
}

bool hasRedFlag(const vector<string>& symptoms) {
    const vector<string> redFlags = {"chest pain", "slurred speech", "facial drooping",
                                     "weakness on one side", "loss of consciousness"};
    for (auto& s : symptoms)
        if (find(redFlags.begin(), redFlags.end(), s) != redFlags.end()) return true;
    return false;
}

Severity classifySeverity(const Condition& top, double score, int matchedCount, bool redFlag) {
    if (redFlag) return EMERGENCY;
    if (top.getBaseSeverity() == EMERGENCY && score >= 0.40) return EMERGENCY;
    if (top.getBaseSeverity() >= HIGH)
        return (score >= 0.50) ? HIGH : MODERATE;
    if (top.getBaseSeverity() == MODERATE)
        return (score >= 0.40) ? MODERATE : LOW;
    return (matchedCount >= 4) ? MODERATE : LOW;
}