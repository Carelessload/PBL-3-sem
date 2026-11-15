#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;

class MediRouteException : public runtime_error {
public:
    MediRouteException(const string& m) : runtime_error(m) {}
};
class UnrecognizedSymptomException : public MediRouteException {
public:
    UnrecognizedSymptomException(const string& s)
        : MediRouteException("Unrecognized symptom(s): " + s) {}
};
class NoMatchException : public MediRouteException {
public:
    NoMatchException(const string& s) : MediRouteException("No matching condition found: " + s) {}
};
class EmptyDoctorResultException : public MediRouteException {
public:
    EmptyDoctorResultException(const string& s) : MediRouteException("No doctors found for specialty: " + s) {}
};
class InvalidInputException : public MediRouteException {
public:
    InvalidInputException(const string& s) : MediRouteException(s) {}
};

enum Severity { LOW, MODERATE, HIGH, EMERGENCY };

string severityName(Severity s) {
    const char* names[] = {"LOW", "MODERATE", "HIGH", "EMERGENCY"};
    return names[s];
}

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

string safeField(string s) {
    replace(s.begin(), s.end(), '|', ' ');
    return trim(s);
}

string normalize(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (isspace((unsigned char)s[i])) {
            if (!out.empty() && out.back() != ' ') out += ' ';
        } else {
            out += (char)tolower((unsigned char)s[i]);
        }
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

string join(const vector<string>& v, const string& sep) {
    string r;
    for (size_t i = 0; i < v.size(); i++) r += (i ? sep : "") + v[i];
    return r;
}

vector<string> split(const string& s, char d) {
    vector<string> out;
    string part;
    stringstream ss(s);
    while (getline(ss, part, d)) out.push_back(part);
    return out;
}

string num(double v, int precision) {
    ostringstream o;
    o << fixed << setprecision(precision) << v;
    return o.str();
}

string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) throw runtime_error("Input closed");
    return trim(s);
}

string readNonEmpty(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        if (!s.empty()) return s;
        cout << "  This field cannot be empty.\n";
    }
}

int readInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t used = 0;
            int v = stoi(s, &used);
            if (used == s.size() && v >= lo && v <= hi) return v;
        } catch (const exception&) {}
        cout << "  Please enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

double readDouble(const string& prompt, double lo, double hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t used = 0;
            double v = stod(s, &used);
            if (used == s.size() && v >= lo && v <= hi) return v;
        } catch (const exception&) {}
        cout << "  Please enter a number from " << lo << " to " << hi << ".\n";
    }
}

class Symptom {
    string name;
    bool redFlag;
public:
    Symptom(const string& n, bool rf) : name(n), redFlag(rf) {
        if (n.empty()) throw InvalidInputException("Symptom name is empty.");
    }
    string getName() const { return name; }
    bool isRedFlag() const { return redFlag; }
};

class Condition {
    string name, specialist, advice, causes, tests;
    Severity baseSeverity;
    map<string, int> weights;
    int totalWeight;
public:
    Condition(const string& n, const string& sp, Severity sev, const string& adv,
              const string& cau, const string& tst)
        : name(n), specialist(sp), advice(adv), causes(cau), tests(tst),
          baseSeverity(sev), totalWeight(0) {
        if (n.empty() || sp.empty()) throw InvalidInputException("Condition needs a name and a specialist.");
    }
    void addSymptom(const string& s, int w) {
        if (w < 1 || w > 5) throw InvalidInputException("Symptom weight must be 1 to 5.");
        if (weights.count(s)) totalWeight -= weights[s];
        weights[s] = w;
        totalWeight += w;
    }
    int weightOf(const string& s) const {
        map<string, int>::const_iterator it = weights.find(s);
        return it == weights.end() ? 0 : it->second;
    }
    string getName() const { return name; }
    string getSpecialist() const { return specialist; }
    string getAdvice() const { return advice; }
    string getCauses() const { return causes; }
    string getTests() const { return tests; }
    Severity getSeverity() const { return baseSeverity; }
    int getTotalWeight() const { return totalWeight; }
    const map<string, int>& getSymptoms() const { return weights; }
};

class Address {
    string line, area, city;
    string areaKey, cityKey;

    static bool similar(const string& a, const string& b) {
        if (a.empty() || b.empty()) return false;
        if (a == b) return true;
        return min(a.size(), b.size()) >= 3 &&
               (a.find(b) != string::npos || b.find(a) != string::npos);
    }
public:
    Address(const string& l, const string& a, const string& c)
        : line(safeField(l)), area(safeField(a)), city(safeField(c)) {
        areaKey = normalize(area);
        cityKey = normalize(city);
        if (areaKey.empty()) throw InvalidInputException("Area / locality is empty.");
        if (cityKey.empty()) throw InvalidInputException("City is empty.");
    }
    string getLine() const { return line; }
    string getArea() const { return area; }
    string getCity() const { return city; }

    string toString() const { return (line.empty() ? "" : line + ", ") + area + ", " + city; }

    double nearness(const Address& other) const {
        if (!similar(cityKey, other.cityKey)) return 0.0;
        return similar(areaKey, other.areaKey) ? 1.0 : 0.5;
    }
};

string nearnessLabel(double n) {
    if (n >= 1.0) return "Same area";
    if (n >= 0.5) return "Same city";
    return "Other city";
}

class Patient {
    string name;
    int age;
    Address address;
public:
    Patient(const string& n, int a, const Address& addr) : age(0), address(addr) {
        setName(n);
        setAge(a);
    }
    void setName(const string& n) {
        if (n.empty()) throw InvalidInputException("Patient name is empty.");
        name = n;
    }
    void setAge(int a) {
        if (a < 0 || a > 120) throw InvalidInputException("Age must be 0 to 120.");
        age = a;
    }
    void setAddress(const Address& a) { address = a; }
    string getName() const { return name; }
    int getAge() const { return age; }
    const Address& getAddress() const { return address; }
};

class Doctor {
    string name, specialty, hospital;
    int experience;
    double rating;
    bool available;
    Address address;
public:
    Doctor(const string& n, const string& sp, int exp, double r, bool av,
           const string& h, const Address& addr)
        : name(n), specialty(sp), hospital(h), experience(exp), rating(r),
          available(av), address(addr) {
        if (n.empty() || sp.empty()) throw InvalidInputException("Doctor needs a name and a specialty.");
        if (exp < 0 || exp > 60) throw InvalidInputException("Experience must be 0 to 60 years.");
        if (r < 0.0 || r > 5.0) throw InvalidInputException("Rating must be 0 to 5.");
    }
    string getName() const { return name; }
    string getSpecialty() const { return specialty; }
    string getHospital() const { return hospital; }
    int getExperience() const { return experience; }
    double getRating() const { return rating; }
    bool isAvailable() const { return available; }
    const Address& getAddress() const { return address; }
    void setAvailable(bool a) { available = a; }
};

class Hospital {
    string name;
    Address address;
    bool emergency;
public:
    Hospital(const string& n, const Address& a, bool em) : name(n), address(a), emergency(em) {
        if (n.empty()) throw InvalidInputException("Hospital name is empty.");
    }
    string getName() const { return name; }
    const Address& getAddress() const { return address; }
    bool hasEmergency() const { return emergency; }
};

Address readAddress() {
    string line = readLine("  House no. / street (optional): ");
    string area = readNonEmpty("  Area / locality (e.g. Rajpur Road): ");
    string city = readNonEmpty("  City (e.g. Dehradun): ");
    return Address(line, area, city);
}

class TrieNode {
public:
    map<char, TrieNode*> next;
    bool isEnd;
    TrieNode() : isEnd(false) {}
    ~TrieNode() { for (auto& p : next) delete p.second; }
    TrieNode(const TrieNode&) = delete;
    TrieNode& operator=(const TrieNode&) = delete;
};

class Trie {
    TrieNode* root;
    void collect(const TrieNode* n, const string& cur, vector<string>& out) const {
        if (n->isEnd) out.push_back(cur);
        for (auto& p : n->next) collect(p.second, cur + p.first, out);
    }
public:
    Trie() : root(new TrieNode()) {}
    ~Trie() { delete root; }
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const string& word) {
        TrieNode* n = root;
        for (char c : word) {
            if (!n->next.count(c)) n->next[c] = new TrieNode();
            n = n->next[c];
        }
        n->isEnd = true;
    }
    vector<string> startsWith(const string& prefix) const {
        vector<string> out;
        const TrieNode* n = root;
        for (char c : prefix) {
            map<char, TrieNode*>::const_iterator it = n->next.find(c);
            if (it == n->next.end()) return out;
            n = it->second;
        }
        collect(n, prefix, out);
        return out;
    }
};

class SymptomMatcher {
    Trie trie;
    unordered_map<string, vector<int>> index;
    unordered_map<string, string> synonyms;

public:
    SymptomMatcher() {
        synonyms = {
            {"high temperature", "fever"}, {"temperature", "fever"}, {"tired", "fatigue"},
            {"tiredness", "fatigue"}, {"weakness", "fatigue"}, {"throwing up", "vomiting"},
            {"vomit", "vomiting"}, {"loose motion", "diarrhea"}, {"loose motions", "diarrhea"},
            {"tummy ache", "stomach pain"}, {"stomach ache", "stomach pain"},
            {"belly pain", "stomach pain"}, {"breathlessness", "shortness of breath"},
            {"difficulty breathing", "shortness of breath"}, {"dizzy", "dizziness"},
            {"itchy", "itching"}, {"rash", "skin rash"}, {"body pain", "body ache"},
            {"heart pain", "chest pain"}, {"stuffy nose", "nasal congestion"},
            {"blocked nose", "nasal congestion"}, {"joint ache", "joint pain"},
            {"thirsty", "excessive thirst"}, {"fainting", "loss of consciousness"},
            {"passed out", "loss of consciousness"}};
    }

    void build(const vector<Condition>& conditions) {
        for (size_t i = 0; i < conditions.size(); i++)
            for (auto& s : conditions[i].getSymptoms()) {
                trie.insert(s.first);
                index[s.first].push_back((int)i);
            }
    }

    const vector<int>& conditionsFor(const string& symptom) const {
        static const vector<int> none;
        unordered_map<string, vector<int>>::const_iterator it = index.find(symptom);
        return it == index.end() ? none : it->second;
    }

    vector<string> parse(const string& raw) const {
        string line = normalize(raw);
        replace(line.begin(), line.end(), ';', ',');
        size_t pos;
        while ((pos = line.find(" and ")) != string::npos) line.replace(pos, 5, ",");
        vector<string> tokens;
        for (const string& part : split(line, ',')) {
            string t = normalize(part);
            if (!t.empty() && find(tokens.begin(), tokens.end(), t) == tokens.end()) tokens.push_back(t);
        }
        return tokens;
    }

    string resolve(const string& token) const {
        string t = token;
        unordered_map<string, string>::const_iterator syn = synonyms.find(t);
        if (syn != synonyms.end()) t = syn->second;
        if (index.count(t)) return t;
        vector<string> options = trie.startsWith(t);
        return options.size() == 1 ? options[0] : "";
    }

    vector<string> match(const string& line, vector<string>& unknown) const {
        vector<string> found;
        for (const string& tok : parse(line)) {
            string r = resolve(tok);
            if (r.empty()) unknown.push_back(tok);
            else if (find(found.begin(), found.end(), r) == found.end()) found.push_back(r);
        }
        if (found.empty())
            throw UnrecognizedSymptomException(unknown.empty() ? string("(nothing entered)") : join(unknown, ", "));
        return found;
    }

    vector<string> suggest(const string& prefix) const { return trie.startsWith(normalize(prefix)); }
};

struct Ranked {
    int idx;
    double score;
    int matched;
    int base;
};

struct RankLess {
    bool operator()(const Ranked& a, const Ranked& b) const {
        if (fabs(a.score - b.score) > 1e-9) return a.score < b.score;
        if (a.base != b.base) return a.base < b.base;
        return a.idx > b.idx;
    }
};

vector<Ranked> rankConditions(const vector<Condition>& cs, const SymptomMatcher& m,
                              const vector<string>& symptoms, size_t topK) {
    map<int, pair<int, int>> hit;
    for (const string& s : symptoms)
        for (int idx : m.conditionsFor(s)) {
            hit[idx].first += cs[idx].weightOf(s);
            hit[idx].second++;
        }

    priority_queue<Ranked, vector<Ranked>, RankLess> heap;
    for (auto& h : hit) {
        Ranked r;
        r.idx = h.first;
        r.matched = h.second.second;
        r.base = (int)cs[r.idx].getSeverity();
        r.score = 0.7 * h.second.first / cs[r.idx].getTotalWeight() + 0.3 * r.matched / symptoms.size();
        heap.push(r);
    }
    if (heap.empty()) throw NoMatchException(symptoms.empty() ? "(none)" : symptoms[0]);

    vector<Ranked> top;
    while (!heap.empty() && top.size() < topK) { top.push_back(heap.top()); heap.pop(); }
    return top;
}

struct Context {
    Severity base;
    double score;
    bool redFlag;
};

class DecisionNode {
public:
    string question;
    function<bool(const Context&)> test;
    DecisionNode *yes, *no;
    Severity result;
    bool isLeaf;

    DecisionNode() : yes(NULL), no(NULL), result(LOW), isLeaf(false) {}
    ~DecisionNode() { delete yes; delete no; }
    DecisionNode(const DecisionNode&) = delete;
    DecisionNode& operator=(const DecisionNode&) = delete;

    static DecisionNode* leaf(Severity s) {
        DecisionNode* n = new DecisionNode();
        n->isLeaf = true;
        n->result = s;
        return n;
    }
    static DecisionNode* ask(const string& q, function<bool(const Context&)> t,
                             DecisionNode* yes, DecisionNode* no) {
        DecisionNode* n = new DecisionNode();
        n->question = q;
        n->test = t;
        n->yes = yes;
        n->no = no;
        return n;
    }
};

class SeverityClassifier {
    DecisionNode* root;
public:
    SeverityClassifier() {
        DecisionNode* moderate = DecisionNode::ask("score >= 0.40?",
            [](const Context& c) { return c.score >= 0.40; },
            DecisionNode::leaf(MODERATE), DecisionNode::leaf(LOW));
        DecisionNode* isModerate = DecisionNode::ask("base severity is MODERATE?",
            [](const Context& c) { return c.base == MODERATE; },
            moderate, DecisionNode::leaf(LOW));
        DecisionNode* high = DecisionNode::ask("score >= 0.50?",
            [](const Context& c) { return c.score >= 0.50; },
            DecisionNode::leaf(HIGH), DecisionNode::leaf(MODERATE));
        DecisionNode* isHigh = DecisionNode::ask("base severity HIGH?",
            [](const Context& c) { return c.base == HIGH; },
            high, isModerate);
        DecisionNode* emergencyCond = DecisionNode::ask("EMERGENCY condition with score >= 0.40?",
            [](const Context& c) { return c.base == EMERGENCY && c.score >= 0.40; },
            DecisionNode::leaf(EMERGENCY), isHigh);
        root = DecisionNode::ask("red-flag symptom present?",
            [](const Context& c) { return c.redFlag; },
            DecisionNode::leaf(EMERGENCY), emergencyCond);
    }
    ~SeverityClassifier() { delete root; }
    SeverityClassifier(const SeverityClassifier&) = delete;
    SeverityClassifier& operator=(const SeverityClassifier&) = delete;

    Severity classify(const Context& ctx, vector<string>* trace = NULL) const {
        const DecisionNode* n = root;
        while (!n->isLeaf) {
            bool ans = n->test(ctx);
            if (trace) trace->push_back(n->question + (ans ? " -> yes" : " -> no"));
            n = ans ? n->yes : n->no;
        }
        return n->result;
    }
};

struct Assessment {
    vector<string> symptoms, unknown, trace;
    vector<Ranked> ranked;
    Severity severity;
};

Assessment assess(const string& input, const SymptomMatcher& matcher,
                  const vector<Condition>& conditions, const unordered_set<string>& redFlags,
                  const SeverityClassifier& classifier) {
    Assessment a;
    a.symptoms = matcher.match(input, a.unknown);
    a.ranked = rankConditions(conditions, matcher, a.symptoms, 3);

    Context ctx;
    ctx.base = conditions[a.ranked[0].idx].getSeverity();
    ctx.score = a.ranked[0].score;
    ctx.redFlag = false;
    for (const string& s : a.symptoms)
        if (redFlags.count(s)) ctx.redFlag = true;

    a.severity = classifier.classify(ctx, &a.trace);
    return a;
}

vector<int> findConditions(const vector<Condition>& cs, const string& query) {
    vector<int> found;
    string q = normalize(query);
    if (q.empty()) return found;
    for (size_t i = 0; i < cs.size(); i++)
        if (normalize(cs[i].getName()).find(q) != string::npos) found.push_back((int)i);
    return found;
}

void showHealthInfo(const Condition& c) {
    vector<pair<int, string>> syms;
    for (auto& s : c.getSymptoms()) syms.push_back(make_pair(-s.second, s.first));
    sort(syms.begin(), syms.end());
    vector<string> names;
    for (auto& s : syms) names.push_back(s.second);

    cout << "\n  ---- Health information: " << c.getName() << " ----\n"
         << "  Usual seriousness : " << severityName(c.getSeverity()) << "\n"
         << "  Common causes     : " << c.getCauses() << "\n"
         << "  Common symptoms   : " << join(names, ", ") << "\n"
         << "  Likely tests      : " << c.getTests() << "\n"
         << "  Doctor to visit   : " << c.getSpecialist() << "\n"
         << "  General care      : " << c.getAdvice() << "\n"
         << "  (General information only - not a diagnosis.)\n";
}

class SpecialistMap {
    unordered_map<string, string> table;
public:
    void build(const vector<Condition>& cs) {
        for (const Condition& c : cs) table[c.getName()] = c.getSpecialist();
    }
    string get(const string& condition) const {
        unordered_map<string, string>::const_iterator it = table.find(condition);
        if (it == table.end()) throw NoMatchException(condition);
        return it->second;
    }
};

struct ScoredDoctor {
    const Doctor* doctor;
    double score;
    double nearness;
};

struct DoctorLess {
    bool operator()(const ScoredDoctor& a, const ScoredDoctor& b) const {
        if (fabs(a.score - b.score) > 1e-9) return a.score < b.score;
        if (a.doctor->getRating() != b.doctor->getRating())
            return a.doctor->getRating() < b.doctor->getRating();
        if (a.nearness != b.nearness) return a.nearness < b.nearness;
        return a.doctor->getName() > b.doctor->getName();
    }
};

class DoctorRanker {
public:
    double specialtyMatch(const Doctor& d, const string& needed) const {
        if (d.getSpecialty() == needed) return 1.0;
        if (d.getSpecialty() == "General Physician") return 0.5;
        return 0.0;
    }

    double score(const Doctor& d, const string& needed, double nearness) const {
        double experience = min(d.getExperience(), 30) / 30.0;
        double rating = d.getRating() / 5.0;
        double available = d.isAvailable() ? 1.0 : 0.0;
        return 0.30 * specialtyMatch(d, needed) + 0.25 * rating + 0.20 * experience +
               0.15 * nearness + 0.10 * available;
    }

    vector<ScoredDoctor> rank(const vector<Doctor>& doctors, const string& specialty,
                              const Address& from, size_t topN) const {
        priority_queue<ScoredDoctor, vector<ScoredDoctor>, DoctorLess> heap;
        for (const Doctor& d : doctors) {
            if (specialtyMatch(d, specialty) == 0.0) continue;
            ScoredDoctor sd;
            sd.doctor = &d;
            sd.nearness = from.nearness(d.getAddress());
            sd.score = score(d, specialty, sd.nearness);
            heap.push(sd);
        }
        if (heap.empty()) throw EmptyDoctorResultException(specialty);

        vector<ScoredDoctor> result;
        while (!heap.empty() && result.size() < topN) { result.push_back(heap.top()); heap.pop(); }
        return result;
    }
};

class ConsultLogger {
    string path;
public:
    ConsultLogger(const string& p) : path(p) {}

    void log(const Patient& p, const vector<string>& symptoms, const string& condition,
             Severity sev, const vector<string>& doctorNames) const {
        ofstream out(path.c_str(), ios::app);
        if (!out) throw MediRouteException("Cannot open log file: " + path);
        time_t now = time(NULL);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        out << "[" << buf << "] patient=" << p.getName() << " (age " << p.getAge() << ")"
            << " | address=" << p.getAddress().toString()
            << " | symptoms=" << join(symptoms, ", ")
            << " | top_condition=" << condition
            << " | severity=" << severityName(sev)
            << " | doctors=" << (doctorNames.empty() ? string("none") : join(doctorNames, "; ")) << "\n";
    }

    void printHistory(size_t n) const {
        ifstream in(path.c_str());
        vector<string> lines;
        string line;
        while (getline(in, line))
            if (!line.empty()) lines.push_back(line);
        if (lines.empty()) { cout << "\n  No consultation history yet.\n"; return; }
        size_t start = lines.size() > n ? lines.size() - n : 0;
        cout << "\n  Last consultations (" << lines.size() - start << " of " << lines.size() << "):\n";
        for (size_t i = start; i < lines.size(); i++) cout << "   " << lines[i] << "\n";
    }
};

vector<Hospital> seedHospitals() {
    vector<Hospital> v;
    v.push_back(Hospital("City Care Hospital",     Address("12 Gandhi Road",   "Clock Tower",  "Dehradun"), true));
    v.push_back(Hospital("Apex Heart Institute",   Address("45 Rajpur Road",   "Rajpur Road",  "Dehradun"), true));
    v.push_back(Hospital("Sunrise Medical Centre", Address("8 Ballupur Chowk", "Ballupur",     "Dehradun"), true));
    v.push_back(Hospital("Green Valley Clinic",    Address("22 Main Market",   "Clement Town", "Dehradun"), false));
    return v;
}

vector<Doctor> seedDoctors(const vector<Hospital>& hs) {
    struct Row { const char* name; const char* spec; int exp; double rating; bool avail; int h; };
    Row rows[] = {
        {"Dr. Meera Sharma",  "General Physician",  12, 4.5, true,  0},
        {"Dr. Rohan Verma",   "General Physician",   6, 4.1, true,  3},
        {"Dr. Sana Iqbal",    "General Physician",   9, 4.3, true,  2},
        {"Dr. Anil Kapoor",   "Cardiologist",       25, 4.8, true,  0},
        {"Dr. Sunita Rao",    "Cardiologist",       15, 4.6, false, 1},
        {"Dr. Vikram Joshi",  "Neurologist",        18, 4.7, true,  1},
        {"Dr. Priya Nair",    "Neurologist",         9, 4.2, true,  3},
        {"Dr. Imran Khan",    "Pulmonologist",      20, 4.6, true,  0},
        {"Dr. Neha Gupta",    "Gastroenterologist", 11, 4.4, true,  2},
        {"Dr. Arvind Menon",  "Endocrinologist",    14, 4.5, true,  2},
        {"Dr. Kavita Singh",  "Dermatologist",       8, 4.3, true,  3},
        {"Dr. Rajesh Pillai", "ENT Specialist",     16, 4.4, false, 0},
        {"Dr. Farah Ali",     "Orthopedist",        13, 4.5, true,  2},
    };
    vector<Doctor> v;
    for (const Row& r : rows) {
        const Hospital& h = hs[r.h];
        v.push_back(Doctor(r.name, r.spec, r.exp, r.rating, r.avail, h.getName(), h.getAddress()));
    }
    return v;
}

void saveHospitals(const vector<Hospital>& hs) {
    ofstream out("hospitals.txt");
    if (!out) throw MediRouteException("Cannot write hospitals.txt");
    for (const Hospital& h : hs)
        out << safeField(h.getName()) << "|" << h.getAddress().getLine() << "|"
            << h.getAddress().getArea() << "|" << h.getAddress().getCity() << "|"
            << (h.hasEmergency() ? 1 : 0) << "\n";
}

void saveDoctors(const vector<Doctor>& ds) {
    ofstream out("doctors.txt");
    if (!out) throw MediRouteException("Cannot write doctors.txt");
    for (const Doctor& d : ds)
        out << safeField(d.getName()) << "|" << safeField(d.getSpecialty()) << "|"
            << d.getExperience() << "|" << d.getRating() << "|" << (d.isAvailable() ? 1 : 0) << "|"
            << safeField(d.getHospital()) << "|" << d.getAddress().getLine() << "|"
            << d.getAddress().getArea() << "|" << d.getAddress().getCity() << "\n";
}

void loadHospitals(vector<Hospital>& hs) {
    ifstream in("hospitals.txt");
    string line;
    int bad = 0;
    while (getline(in, line)) {
        if (trim(line).empty()) continue;
        try {
            vector<string> f = split(line, '|');
            if (f.size() != 5) throw InvalidInputException("bad field count");
            hs.push_back(Hospital(f[0], Address(f[1], f[2], f[3]), stoi(f[4]) == 1));
        } catch (const exception&) { bad++; }
    }
    if (bad) cout << "  (warning: " << bad << " bad line(s) skipped in hospitals.txt)\n";
}

void loadDoctors(vector<Doctor>& ds) {
    ifstream in("doctors.txt");
    string line;
    int bad = 0;
    while (getline(in, line)) {
        if (trim(line).empty()) continue;
        try {
            vector<string> f = split(line, '|');
            if (f.size() != 9) throw InvalidInputException("bad field count");
            ds.push_back(Doctor(f[0], f[1], stoi(f[2]), stod(f[3]), stoi(f[4]) == 1,
                                f[5], Address(f[6], f[7], f[8])));
        } catch (const exception&) { bad++; }
    }
    if (bad) cout << "  (warning: " << bad << " bad line(s) skipped in doctors.txt)\n";
}

void saveAll(const vector<Hospital>& hs, const vector<Doctor>& ds) {
    try { saveHospitals(hs); saveDoctors(ds); }
    catch (const MediRouteException& e) { cout << "  (warning: " << e.what() << ")\n"; }
}

void loadAll(vector<Hospital>& hs, vector<Doctor>& ds) {
    loadHospitals(hs);
    if (hs.empty()) hs = seedHospitals();
    loadDoctors(ds);
    if (ds.empty()) ds = seedDoctors(hs);
    saveAll(hs, ds);
}

vector<string> collectSpecialties(const vector<Condition>& cs) {
    vector<string> list;
    for (const Condition& c : cs)
        if (find(list.begin(), list.end(), c.getSpecialist()) == list.end()) list.push_back(c.getSpecialist());
    return list;
}

void listAllDoctors(const vector<Doctor>& doctors) {
    cout << "\nRegistered doctors (" << doctors.size() << "):\n";
    for (const Doctor& d : doctors)
        cout << "    - " << d.getName() << " | " << d.getSpecialty() << " | " << d.getExperience()
             << " yrs | rating " << num(d.getRating(), 1) << " | " << d.getHospital() << ", "
             << d.getAddress().toString() << " | " << (d.isAvailable() ? "available" : "not available") << "\n";
}

void registerDoctor(vector<Doctor>& doctors, vector<Hospital>& hospitals, const vector<string>& specialties) {
    cout << "\n--- Doctor Registration ---\n";
    string name = safeField(readNonEmpty("Full name: "));
    if (name.rfind("Dr.", 0) != 0 && name.rfind("dr.", 0) != 0) name = "Dr. " + name;

    cout << "Select your specialty:\n";
    for (size_t i = 0; i < specialties.size(); i++) cout << "  " << i + 1 << ". " << specialties[i] << "\n";
    string specialty = specialties[readInt("Enter number: ", 1, (int)specialties.size()) - 1];

    int experience = readInt("Years of experience (0-60): ", 0, 60);
    double rating = readDouble("Your rating (0.0 - 5.0): ", 0.0, 5.0);

    string workplace = safeField(readNonEmpty("Hospital / clinic name: "));
    cout << "Address of your hospital / clinic:\n";
    Address address = readAddress();

    for (const Doctor& d : doctors)
        if (normalize(d.getName()) == normalize(name) && normalize(d.getHospital()) == normalize(workplace)) {
            cout << "  This doctor is already registered at that hospital.\n";
            return;
        }

    bool known = false;
    for (const Hospital& h : hospitals)
        if (normalize(h.getName()) == normalize(workplace)) known = true;
    if (!known) {
        int em = readInt("Does it have emergency services? (1 = yes, 0 = no): ", 0, 1);
        hospitals.push_back(Hospital(workplace, address, em == 1));
    }

    doctors.push_back(Doctor(name, specialty, experience, rating, true, workplace, address));
    saveAll(hospitals, doctors);
    cout << "\n  Registered successfully: " << name << " (" << specialty << ") at " << workplace
         << ", " << address.toString() << "\n  Patients can now see you in their doctor list.\n";
}

void updateAvailability(vector<Doctor>& doctors, const vector<Hospital>& hospitals) {
    string name = normalize(readNonEmpty("\nEnter your name (as registered): "));
    vector<size_t> matches;
    for (size_t i = 0; i < doctors.size(); i++)
        if (normalize(doctors[i].getName()).find(name) != string::npos) matches.push_back(i);
    if (matches.empty()) { cout << "  No doctor found with that name.\n"; return; }

    size_t chosen = matches[0];
    if (matches.size() > 1) {
        for (size_t i = 0; i < matches.size(); i++)
            cout << "  " << i + 1 << ". " << doctors[matches[i]].getName() << " @ " << doctors[matches[i]].getHospital() << "\n";
        chosen = matches[readInt("Which one are you? ", 1, (int)matches.size()) - 1];
    }
    int av = readInt("Available now? (1 = yes, 0 = no): ", 0, 1);
    doctors[chosen].setAvailable(av == 1);
    saveAll(hospitals, doctors);
    cout << "  " << doctors[chosen].getName() << " is now marked " << (av == 1 ? "AVAILABLE" : "NOT available") << ".\n";
}

vector<Symptom> loadSymptoms() {
    vector<Symptom> v;
    v.push_back(Symptom("chest pain", true));
    v.push_back(Symptom("facial drooping", true));
    v.push_back(Symptom("slurred speech", true));
    v.push_back(Symptom("weakness on one side", true));
    v.push_back(Symptom("loss of consciousness", true));
    return v;
}

vector<Condition> loadConditions() {
    vector<Condition> v;

    Condition cold("Common Cold", "General Physician", LOW,
        "Rest, drink warm fluids and gargle with warm salt water.",
        "Viral infection spread by droplets and contact.", "Usually none; a doctor's examination.");
    cold.addSymptom("runny nose", 3); cold.addSymptom("sneezing", 3);
    cold.addSymptom("sore throat", 2); cold.addSymptom("cough", 2);
    v.push_back(cold);

    Condition flu("Influenza", "General Physician", MODERATE,
        "Rest and drink plenty of fluids; see a doctor if it lasts over 3 days.",
        "Influenza virus infection.", "Rapid flu test, clinical examination.");
    flu.addSymptom("fever", 4); flu.addSymptom("body ache", 3); flu.addSymptom("cough", 2);
    flu.addSymptom("fatigue", 3); flu.addSymptom("headache", 2); flu.addSymptom("chills", 3);
    v.push_back(flu);

    Condition migraine("Migraine", "Neurologist", MODERATE,
        "Rest in a dark, quiet room and stay hydrated.",
        "Stress, lack of sleep, some foods and hormonal changes.",
        "Medical history and examination; scans only if needed.");
    migraine.addSymptom("headache", 4); migraine.addSymptom("nausea", 2);
    migraine.addSymptom("light sensitivity", 4); migraine.addSymptom("blurred vision", 2);
    migraine.addSymptom("dizziness", 2);
    v.push_back(migraine);

    Condition gastro("Gastroenteritis", "Gastroenterologist", MODERATE,
        "Take oral rehydration solution and eat light, bland food.",
        "Viral or bacterial infection, contaminated food or water.",
        "Stool test, hydration check, blood tests if severe.");
    gastro.addSymptom("nausea", 3); gastro.addSymptom("vomiting", 4); gastro.addSymptom("diarrhea", 4);
    gastro.addSymptom("stomach pain", 3); gastro.addSymptom("fever", 2);
    v.push_back(gastro);

    Condition asthma("Asthma", "Pulmonologist", HIGH,
        "Sit upright, stay calm and use your inhaler if prescribed.",
        "Airway inflammation triggered by allergens, cold air, exercise or infections.",
        "Spirometry and peak flow test.");
    asthma.addSymptom("wheezing", 4); asthma.addSymptom("shortness of breath", 4);
    asthma.addSymptom("cough", 2); asthma.addSymptom("chest tightness", 3);
    v.push_back(asthma);

    Condition pneumonia("Pneumonia", "Pulmonologist", HIGH,
        "Rest and fluids, but a medical check-up is needed soon.",
        "Bacterial, viral or fungal lung infection.", "Chest X-ray, blood tests, sputum test.");
    pneumonia.addSymptom("fever", 3); pneumonia.addSymptom("cough", 2);
    pneumonia.addSymptom("shortness of breath", 3); pneumonia.addSymptom("chest pain", 2);
    pneumonia.addSymptom("chills", 2); pneumonia.addSymptom("fatigue", 1);
    v.push_back(pneumonia);

    Condition heart("Heart Attack", "Cardiologist", EMERGENCY,
        "Seek emergency care immediately.",
        "Blocked blood flow to the heart, often from coronary artery disease.",
        "ECG, troponin blood test, angiography.");
    heart.addSymptom("chest pain", 5); heart.addSymptom("shortness of breath", 3);
    heart.addSymptom("left arm pain", 4); heart.addSymptom("sweating", 3);
    heart.addSymptom("loss of consciousness", 2);
    v.push_back(heart);

    Condition stroke("Stroke", "Neurologist", EMERGENCY,
        "Seek emergency care immediately.", "Blocked or burst blood vessel in the brain.",
        "CT or MRI brain scan, blood tests, ECG.");
    stroke.addSymptom("facial drooping", 5); stroke.addSymptom("slurred speech", 5);
    stroke.addSymptom("weakness on one side", 5); stroke.addSymptom("sudden confusion", 3);
    stroke.addSymptom("severe headache", 3); stroke.addSymptom("loss of consciousness", 3);
    v.push_back(stroke);

    Condition hyper("Hypertension", "Cardiologist", MODERATE,
        "Reduce salt, rest, avoid stress and get your blood pressure measured.",
        "Genetics, high salt intake, stress, obesity and lack of exercise.",
        "Repeated blood pressure readings, blood and urine tests, ECG.");
    hyper.addSymptom("headache", 2); hyper.addSymptom("dizziness", 3);
    hyper.addSymptom("blurred vision", 2); hyper.addSymptom("nosebleed", 2);
    v.push_back(hyper);

    Condition diabetes("Type 2 Diabetes", "Endocrinologist", MODERATE,
        "Avoid sugary food and drinks and get a blood sugar test done.",
        "Insulin resistance linked to genetics, weight and lifestyle.",
        "Fasting blood sugar, HbA1c, glucose tolerance test.");
    diabetes.addSymptom("frequent urination", 4); diabetes.addSymptom("excessive thirst", 4);
    diabetes.addSymptom("fatigue", 2); diabetes.addSymptom("blurred vision", 2);
    diabetes.addSymptom("weight loss", 2);
    v.push_back(diabetes);

    Condition eczema("Eczema", "Dermatologist", LOW,
        "Moisturize regularly, avoid harsh soaps and do not scratch the area.",
        "Skin barrier problems, allergies, irritants and genetics.",
        "Skin examination; patch test if needed.");
    eczema.addSymptom("itching", 4); eczema.addSymptom("skin rash", 4);
    eczema.addSymptom("dry skin", 3); eczema.addSymptom("redness", 2);
    v.push_back(eczema);

    Condition rhinitis("Allergic Rhinitis", "ENT Specialist", LOW,
        "Avoid dust and known allergens and rinse your nose with saline.",
        "Allergy to dust, pollen, pets or mould.", "Allergy skin-prick test, IgE blood test.");
    rhinitis.addSymptom("sneezing", 4); rhinitis.addSymptom("runny nose", 3);
    rhinitis.addSymptom("itchy eyes", 3); rhinitis.addSymptom("nasal congestion", 3);
    v.push_back(rhinitis);

    Condition arthritis("Arthritis", "Orthopedist", MODERATE,
        "Gentle movement, a warm compress and rest for the affected joint.",
        "Joint wear and tear, autoimmune disease, ageing or injury.",
        "X-ray, blood tests (ESR, CRP), joint fluid test.");
    arthritis.addSymptom("joint pain", 4); arthritis.addSymptom("stiffness", 3);
    arthritis.addSymptom("swelling", 3);
    v.push_back(arthritis);

    return v;
}

unordered_set<string> redFlagSet(const vector<Symptom>& registry) {
    unordered_set<string> s;
    for (const Symptom& sym : registry)
        if (sym.isRedFlag()) s.insert(sym.getName());
    return s;
}

static int g_run = 0, g_failed = 0;

void check(bool ok, const string& name) {
    g_run++;
    if (!ok) g_failed++;
    cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << name << "\n";
}

void runSelfTests() {
    g_run = g_failed = 0;
    vector<Condition> conditions = loadConditions();
    unordered_set<string> flags = redFlagSet(loadSymptoms());
    SeverityClassifier classifier;
    SymptomMatcher matcher;
    matcher.build(conditions);
    SpecialistMap specMap;
    specMap.build(conditions);

    auto sev = [&](const string& line) { return assess(line, matcher, conditions, flags, classifier).severity; };

    Address home("", "Clock Tower", "Dehradun");

    cout << "\n=== Yuvraj: classes, validation, addresses ===\n";
    {
        bool t1 = false, t2 = false, t3 = false, t4 = false;
        try { Patient p("Test", 200, home); } catch (const InvalidInputException&) { t1 = true; }
        try { Doctor d("Dr. X", "Cardiologist", 5, 6.5, true, "H", home); } catch (const InvalidInputException&) { t2 = true; }
        try { Condition c("A", "B", LOW, "", "", ""); c.addSymptom("x", 9); } catch (const InvalidInputException&) { t3 = true; }
        try { Address a("12 Street", "", "Dehradun"); } catch (const InvalidInputException&) { t4 = true; }
        check(t1, "Patient rejects age 200");
        check(t2, "Doctor rejects rating 6.5");
        check(t3, "Condition rejects symptom weight 9");
        check(t4, "Address rejects an empty area");

        check(home.nearness(Address("5 Gandhi Road", "clock tower", "DEHRADUN")) == 1.0, "same area (ignoring case) -> 1.0");
        check(home.nearness(Address("", "Rajpur Road", "Dehradun")) == 0.5, "same city, other area -> 0.5");
        check(home.nearness(Address("", "Clock Tower", "Delhi")) == 0.0, "different city -> 0.0");
        check(Address("", "Rajpur", "Dehradun").nearness(Address("", "Rajpur Road", "Dehradun")) == 1.0,
              "partial area 'Rajpur' matches 'Rajpur Road'");
    }

    cout << "\n=== Ankit: symptom matching ===\n";
    {
        vector<string> t = matcher.parse("  Fever,  COUGH and   headache ");
        check(t.size() == 3 && t[0] == "fever" && t[2] == "headache", "parse 'Fever, COUGH and headache' into 3 tokens");

        vector<string> unk, r = matcher.match("tummy ache", unk);
        check(r.size() == 1 && r[0] == "stomach pain", "synonym 'tummy ache' -> stomach pain");

        unk.clear();
        r = matcher.match("shortness", unk);
        check(r.size() == 1 && r[0] == "shortness of breath", "unique prefix 'shortness' resolved by trie");

        unk.clear();
        r = matcher.match("chest, fever", unk);
        check(r.size() == 1 && r[0] == "fever" && unk.size() == 1, "ambiguous prefix 'chest' is NOT guessed");

        unk.clear();
        check(matcher.match("fever, fever, Fever", unk).size() == 1, "duplicate symptoms are counted once");

        bool threw = false;
        try { unk.clear(); matcher.match("xyzabc", unk); } catch (const UnrecognizedSymptomException&) { threw = true; }
        check(threw, "unknown symptom throws UnrecognizedSymptomException");

        threw = false;
        try { unk.clear(); matcher.match("   ", unk); } catch (const UnrecognizedSymptomException&) { threw = true; }
        check(threw, "empty input throws UnrecognizedSymptomException");
    }

    cout << "\n=== Neeraj: inference, severity, health info ===\n";
    {
        vector<string> unk, s = matcher.match("fever, cough, headache", unk);
        vector<Ranked> r = rankConditions(conditions, matcher, s, 3);
        check(conditions[r[0].idx].getName() == "Influenza", "fever+cough+headache -> Influenza ranked first");
        check(r.size() == 3 && r[0].score >= r[1].score && r[1].score >= r[2].score, "top-3 are in descending score order");

        check(sev("chest pain, sweating") == EMERGENCY, "red flag 'chest pain' -> EMERGENCY");
        check(sev("slurred speech") == EMERGENCY, "red flag 'slurred speech' -> EMERGENCY");
        check(sev("left arm pain, sweating") == EMERGENCY, "heart-attack pattern without red flag -> EMERGENCY");
        check(sev("wheezing, shortness of breath, cough") == HIGH, "asthma pattern -> HIGH");
        check(sev("fever, cough, headache") == MODERATE, "influenza pattern -> MODERATE");
        check(sev("runny nose, sneezing") == LOW, "cold pattern -> LOW");
        check(sev("itching, skin rash") == LOW, "eczema pattern -> LOW");

        bool threw = false;
        try { vector<string> none; rankConditions(conditions, matcher, none, 3); } catch (const NoMatchException&) { threw = true; }
        check(threw, "no symptoms -> NoMatchException");
        check(findConditions(conditions, "DIAB").size() == 1, "health lookup 'DIAB' finds Type 2 Diabetes");
        check(findConditions(conditions, "zzz").empty(), "health lookup of unknown name is empty");
    }

    cout << "\n=== Aditi: specialist map, doctor ranking, log file ===\n";
    {
        check(specMap.get("Asthma") == "Pulmonologist", "Asthma -> Pulmonologist");
        bool threw = false;
        try { specMap.get("Unknown disease"); } catch (const NoMatchException&) { threw = true; }
        check(threw, "unknown condition -> NoMatchException");

        DoctorRanker ranker;

        vector<Doctor> docs;
        docs.push_back(Doctor("Dr. B", "Cardiologist", 5, 3.5, true, "H", home));
        docs.push_back(Doctor("Dr. C", "General Physician", 30, 5.0, true, "H", home));
        docs.push_back(Doctor("Dr. A", "Cardiologist", 20, 4.8, true, "H", home));
        docs.push_back(Doctor("Dr. D", "Dermatologist", 20, 5.0, true, "H", home));
        vector<ScoredDoctor> r = ranker.rank(docs, "Cardiologist", home, 10);
        check(r.size() == 3, "unrelated specialty (Dermatologist) is excluded");
        check(r[0].doctor->getName() == "Dr. A" && r[2].doctor->getName() == "Dr. B",
              "order: best specialist, then GP fallback, then weaker specialist");
        check(ranker.rank(docs, "Cardiologist", home, 1).size() == 1, "top-1 returns exactly one doctor");

        vector<Doctor> tie;
        tie.push_back(Doctor("Dr. Zed", "Neurologist", 10, 4.0, true, "H", home));
        tie.push_back(Doctor("Dr. Amy", "Neurologist", 10, 4.0, true, "H", home));
        check(ranker.rank(tie, "Neurologist", home, 5)[0].doctor->getName() == "Dr. Amy", "tie-break -> name A-Z");

        vector<Doctor> av;
        av.push_back(Doctor("Dr. Off", "Neurologist", 10, 4.0, false, "H", home));
        av.push_back(Doctor("Dr. On", "Neurologist", 10, 4.0, true, "H", home));
        check(ranker.rank(av, "Neurologist", home, 5)[0].doctor->getName() == "Dr. On", "available doctor ranks higher");

        vector<Doctor> place;
        place.push_back(Doctor("Dr. OtherCity", "Neurologist", 10, 4.0, true, "H", Address("", "Clock Tower", "Delhi")));
        place.push_back(Doctor("Dr. SameCity", "Neurologist", 10, 4.0, true, "H", Address("", "Rajpur Road", "Dehradun")));
        place.push_back(Doctor("Dr. SameArea", "Neurologist", 10, 4.0, true, "H", Address("", "Clock Tower", "Dehradun")));
        r = ranker.rank(place, "Neurologist", home, 5);
        check(r[0].doctor->getName() == "Dr. SameArea" && r[1].doctor->getName() == "Dr. SameCity" &&
              r[2].doctor->getName() == "Dr. OtherCity", "ranking by address: same area > same city > other city");

        vector<Doctor> onlySkin;
        onlySkin.push_back(Doctor("Dr. S", "Dermatologist", 5, 4.0, true, "H", home));
        threw = false;
        try { ranker.rank(onlySkin, "Cardiologist", home, 5); } catch (const EmptyDoctorResultException&) { threw = true; }
        check(threw, "no matching doctor -> EmptyDoctorResultException");

        ConsultLogger logger("selftest_log.txt");
        Patient p("Tester", 30, home);
        logger.log(p, vector<string>(1, "fever"), "Influenza", MODERATE, vector<string>(1, "Dr. A"));
        ifstream in("selftest_log.txt");
        string line;
        check(getline(in, line) && line.find("severity=MODERATE") != string::npos, "consult log line is written to file");
        in.close();
        remove("selftest_log.txt");
    }

    cout << "\n  RESULT: " << g_run - g_failed << " / " << g_run << " tests passed.\n";
}

class MediRouteApp {
    vector<Condition> conditions;
    vector<Hospital> hospitals;
    vector<Doctor> doctors;
    vector<string> specialties;
    unordered_set<string> redFlags;
    SymptomMatcher matcher;
    SeverityClassifier classifier;
    SpecialistMap specMap;
    DoctorRanker ranker;
    ConsultLogger logger;
    Patient patient;
    bool profileSet;

    void printDoctors(const vector<ScoredDoctor>& docs) const {
        cout << "\n  #  Doctor                Specialty           Exp  Rating  Location    Avail  Score\n"
             << "  --------------------------------------------------------------------------------\n";
        for (size_t i = 0; i < docs.size(); i++) {
            const Doctor& d = *docs[i].doctor;
            cout << "  " << left << setw(3) << i + 1 << setw(22) << d.getName() << setw(20) << d.getSpecialty()
                 << setw(5) << d.getExperience() << setw(8) << num(d.getRating(), 1)
                 << setw(12) << nearnessLabel(docs[i].nearness) << setw(7) << (d.isAvailable() ? "Yes" : "No")
                 << num(docs[i].score, 2) << "\n      @ " << d.getHospital() << ", " << d.getAddress().toString() << "\n";
        }
    }

    void printNearestEmergency() const {
        const Hospital* best = NULL;
        double bestNear = -1;
        for (const Hospital& h : hospitals) {
            if (!h.hasEmergency()) continue;
            double n = patient.getAddress().nearness(h.getAddress());
            if (n > bestNear) { best = &h; bestNear = n; }
        }
        if (best)
            cout << "  Nearest emergency hospital: " << best->getName() << ", "
                 << best->getAddress().toString() << " (" << nearnessLabel(bestNear) << ")\n";
        else
            cout << "  No emergency hospital is listed. Call your local emergency number.\n";
    }

    void safeLog(const vector<string>& symptoms, const string& condition, Severity sev,
                 const vector<string>& doctorNames) const {
        try { logger.log(patient, symptoms, condition, sev, doctorNames); }
        catch (const MediRouteException& e) { cout << "  (warning: " << e.what() << ")\n"; }
    }

    void consult() {
        string line = readLine("\nEnter your symptoms (e.g. fever, cough and headache):\n> ");
        try {
            Assessment a = assess(line, matcher, conditions, redFlags, classifier);

            cout << "\n  Understood symptoms: " << join(a.symptoms, ", ") << "\n";
            if (!a.unknown.empty()) {
                cout << "  Ignored (not recognized): " << join(a.unknown, ", ") << "\n";
                for (const string& u : a.unknown) {
                    vector<string> sug = matcher.suggest(u.substr(0, 3));
                    if (!sug.empty()) cout << "    did you mean: " << join(sug, ", ") << " ?\n";
                }
            }

            cout << "\n  Possible conditions:\n";
            for (size_t i = 0; i < a.ranked.size(); i++)
                cout << "    " << i + 1 << ". " << conditions[a.ranked[i].idx].getName()
                     << "  (match " << (int)(a.ranked[i].score * 100) << "%)\n";

            const Condition& top = conditions[a.ranked[0].idx];
            cout << "\n  RISK LEVEL: " << severityName(a.severity) << "\n";
            for (const string& step : a.trace) cout << "    . " << step << "\n";

            vector<string> doctorNames;

            if (a.severity == EMERGENCY) {
                cout << "\n  !!! Your symptoms may indicate a medical emergency.\n"
                     << "  !!! Seek immediate medical care / call your local emergency number NOW.\n";
                printNearestEmergency();
                safeLog(a.symptoms, top.getName(), a.severity, doctorNames);
                return;
            }

            if (a.severity == HIGH) cout << "\n  Please consult a doctor soon.\n";
            else cout << "\n  General care: " << top.getAdvice() << "\n";

            string specialty = specMap.get(top.getName());
            cout << "\n  Recommended specialist: " << specialty << "\n";
            try {
                vector<ScoredDoctor> docs = ranker.rank(doctors, specialty, patient.getAddress(), 5);
                printDoctors(docs);
                for (const ScoredDoctor& d : docs) doctorNames.push_back(d.doctor->getName());
            } catch (const EmptyDoctorResultException& e) {
                cout << "\n  " << e.what() << "\n";
            }

            safeLog(a.symptoms, top.getName(), a.severity, doctorNames);

            string ans = normalize(readLine("\n  Show health information about " + top.getName() + "? (y/n): "));
            if (ans == "y" || ans == "yes") showHealthInfo(top);
            cout << "\n  (Preliminary guidance only, not a diagnosis. Please consult a qualified doctor.)\n";

        } catch (const UnrecognizedSymptomException& e) {
            cout << "\n  " << e.what() << "\n  Tip: use option 2 to see the supported symptoms.\n";
        } catch (const MediRouteException& e) {
            cout << "\n  Error: " << e.what() << "\n";
        }
    }

    void browseSymptoms() const {
        string prefix = readLine("\nType the first letters of a symptom (ENTER = list all): ");
        vector<string> list = matcher.suggest(prefix);
        if (list.empty()) { cout << "  No symptom starts with '" << prefix << "'.\n"; return; }
        for (const string& s : list) cout << "   - " << s << "\n";
    }

    void healthLookup() const {
        string q = readNonEmpty("\nCondition name (or part of it, e.g. diab): ");
        vector<int> found = findConditions(conditions, q);
        if (found.empty()) {
            cout << "  Not found. Available conditions:\n";
            for (const Condition& c : conditions) cout << "   - " << c.getName() << "\n";
            return;
        }
        int pick = found[0];
        if (found.size() > 1) {
            for (size_t i = 0; i < found.size(); i++) cout << "  " << i + 1 << ". " << conditions[found[i]].getName() << "\n";
            pick = found[readInt("Which one? ", 1, (int)found.size()) - 1];
        }
        showHealthInfo(conditions[pick]);
    }

    void editProfile() {
        cout << "\n--- Patient profile ---\n";
        patient.setName(readNonEmpty("Your name: "));
        patient.setAge(readInt("Your age: ", 0, 120));
        cout << "Your address (used to find nearby doctors):\n";
        patient.setAddress(readAddress());
        profileSet = true;
        cout << "  Profile saved for this session.\n";
    }

    void patientMenu() {
        if (!profileSet) editProfile();
        while (true) {
            cout << "\n------ PATIENT MENU (" << patient.getName() << ") ------\n"
                 << " 1. Check my symptoms\n 2. Show supported symptoms\n 3. Health information lookup\n"
                 << " 4. View all registered doctors\n 5. My consultation history\n"
                 << " 6. Update my profile / address\n 0. Back to main menu\n";
            int c = readInt("Choice: ", 0, 6);
            if (c == 1) consult();
            else if (c == 2) browseSymptoms();
            else if (c == 3) healthLookup();
            else if (c == 4) listAllDoctors(doctors);
            else if (c == 5) logger.printHistory(10);
            else if (c == 6) editProfile();
            else return;
        }
    }

    void doctorMenu() {
        while (true) {
            cout << "\n------ DOCTOR MENU ------\n"
                 << " 1. Register my details\n 2. Update my availability\n"
                 << " 3. View all registered doctors\n 0. Back to main menu\n";
            int c = readInt("Choice: ", 0, 3);
            if (c == 1) registerDoctor(doctors, hospitals, specialties);
            else if (c == 2) updateAvailability(doctors, hospitals);
            else if (c == 3) listAllDoctors(doctors);
            else return;
        }
    }

public:
    MediRouteApp()
        : conditions(loadConditions()), redFlags(redFlagSet(loadSymptoms())),
          logger("consult_log.txt"), patient("Guest", 25, Address("", "Clock Tower", "Dehradun")),
          profileSet(false) {
        specialties = collectSpecialties(conditions);
        matcher.build(conditions);
        specMap.build(conditions);
        loadAll(hospitals, doctors);
    }

    void run() {
        while (true) {
            cout << "\n====== MediRoute: Smart Healthcare and Symptoms Analysis ======\n"
                 << " Who are you?\n 1. Patient\n 2. Doctor\n 3. Run self-tests\n 0. Exit\n";
            int role = readInt("Choice: ", 0, 3);
            if (role == 1) patientMenu();
            else if (role == 2) doctorMenu();
            else if (role == 3) runSelfTests();
            else { cout << "Stay healthy!\n"; return; }
        }
    }
};

int main() {
    try {
        MediRouteApp app;
        app.run();
    } catch (const exception& e) {
        cout << "\nProgram ended: " << e.what() << "\n";
    }
    return 0;
}