/*
 * Program name:   EECS 348 Assignment 3 - CEO Email Prioritization (C++ objects)
 * Description:    Reads a test file of commands and keeps the CEO's unread
 *                 emails in a priority queue built on a MaxHeap written from
 *                 scratch (list-based, stored in a std::vector). Emails are
 *                 ordered by sender category (Boss > Subordinate > Peer >
 *                 ImportantPerson > OtherPerson) and, within a category, by
 *                 date with the newest email first.
 * Inputs:         A test file of commands, one per line:
 *                     EMAIL <sender category>,<subject line>,<MM-DD-YYYY>
 *                     NEXT
 *                     READ
 *                     COUNT
 *                 The file name is given as the first command-line argument
 *                 (./Assignment3.exe test.txt). With no argument the commands
 *                 are read from standard input (./Assignment3.exe < test.txt).
 * Output:         Terminal output: a "Next email:" block for each NEXT and a
 *                 "There are N emails to read." line for each COUNT, with a
 *                 blank line between blocks. EMAIL and READ print nothing, and
 *                 neither do NEXT or READ when the inbox is empty. Problems with
 *                 the input (missing file, malformed EMAIL line) go to stderr.
 * Collaborators:  None
 * Other sources:  Google Gemma 4 12B (google/gemma-4-12b-qat, run locally in
 *                 LM Studio) generated the base program this file started from.
 *                 Anthropic's Claude Opus 5.5 assisted with the revisions.
 * Author:         David Joslin
 * Creation date:  10/01/2026
 * Revision date:  10/01/2026
 * Revisions:      Fixed the command-line file being closed before it was read;
 *                 strip '\r' and trailing spaces so Windows-style test files
 *                 work; validate EMAIL lines instead of crashing or queueing
 *                 garbage; report a missing input file; store one integer sort
 *                 key per email; moved category names into a single table;
 *                 made Email's data private; added full comments.
 */

#include <fstream>   // std::ifstream for reading the test file
#include <iostream>  // std::cin, std::cout and std::cerr
#include <string>    // std::string for lines and email fields
#include <utility>   // std::swap and std::move
#include <vector>    // std::vector holds the heap's list of emails

// ---------------------------------------------------------------------------
// Sender categories, listed from highest to lowest priority.
// Source: authored by me (Gemma repeated each name in an if/else chain; a
// single table keeps the names and their order in one place).
// ---------------------------------------------------------------------------
const int NUM_CATEGORIES = 5;                       // how many sender categories exist
const char* const CATEGORY_NAMES[NUM_CATEGORIES] = { // index 0 is read first
    "Boss", "Subordinate", "Peer", "ImportantPerson", "OtherPerson"
};

// ---------------------------------------------------------------------------
// Class Email: one message in the CEO's inbox.
// Source: combination. The idea of keeping a numeric category plus a numeric
// YYYYMMDD date for comparisons came from Gemma. Making the fields private,
// folding both numbers into one sort key, and storing the category as an
// index into CATEGORY_NAMES (instead of a copied name string) were authored
// by me.
// ---------------------------------------------------------------------------
class Email {
private:
    int category;        // index into CATEGORY_NAMES (0 = Boss ... 4 = OtherPerson)
    std::string subject; // subject line exactly as it appeared in the file
    std::string date;    // date text as it appeared in the file (MM-DD-YYYY)
    long long key;       // bigger key = read sooner (category first, then date)

public:
    // Constructor: build the email and work out its sort key once.
    // dateNumber is the date as YYYYMMDD so that newer dates are larger.
    Email(int cat, std::string subj, std::string dateText, int dateNumber)
        : category(cat), subject(std::move(subj)), date(std::move(dateText)),
          // flip the category so Boss (0) gets the largest value, then put it
          // above the 8-digit date so category always outweighs the date
          key(static_cast<long long>(NUM_CATEGORIES - 1 - cat) * 100000000LL + dateNumber) {}

    // True if this email should be read before the other one.
    bool outranks(const Email& other) const {
        return key > other.key; // one integer comparison covers category and date
    }

    // Read-only accessors used when printing.
    const char* sender() const { return CATEGORY_NAMES[category]; } // category name
    const std::string& subjectLine() const { return subject; }      // subject text
    const std::string& dateText() const { return date; }            // date text
};

// ---------------------------------------------------------------------------
// Class MaxHeap: list-based max-heap of emails, written from scratch.
// The list is a std::vector where the children of index i are at 2i+1 and
// 2i+2 and its parent is at (i-1)/2, so the highest-priority email is always
// at index 0.
// Source: Gemma (siftUp/siftDown/push/pop/peek/size). I changed pop() to move
// the last email instead of copying it, made peek() return a const pointer,
// and used size_t for indices so they match the vector's size type.
// ---------------------------------------------------------------------------
class MaxHeap {
private:
    std::vector<Email> items; // the heap stored level by level

    // Move the email at idx up until its parent outranks it.
    void siftUp(std::size_t idx) {
        while (idx > 0) {                                // the root has no parent
            std::size_t parent = (idx - 1) / 2;          // index of the parent
            if (!items[idx].outranks(items[parent])) {   // parent is already higher
                break;                                   // heap order restored
            }
            std::swap(items[idx], items[parent]);        // promote the child
            idx = parent;                                // keep checking from its new spot
        }
    }

    // Move the email at idx down until it outranks both of its children.
    void siftDown(std::size_t idx) {
        std::size_t n = items.size();                    // number of emails in the heap
        while (true) {
            std::size_t largest = idx;                   // assume the parent stays on top
            std::size_t left = 2 * idx + 1;              // index of the left child
            std::size_t right = 2 * idx + 2;             // index of the right child
            if (left < n && items[left].outranks(items[largest])) {   // left child is higher
                largest = left;
            }
            if (right < n && items[right].outranks(items[largest])) { // right child is higher still
                largest = right;
            }
            if (largest == idx) {                        // neither child outranks it
                break;                                   // heap order restored
            }
            std::swap(items[idx], items[largest]);       // swap with the higher child
            idx = largest;                               // continue from the child's spot
        }
    }

public:
    // Add an email and restore heap order: O(log n).
    void push(const Email& e) {
        items.push_back(e);          // place it at the end of the list
        siftUp(items.size() - 1);    // bubble it up to its correct level
    }

    // Remove the highest-priority email (does nothing if empty): O(log n).
    void pop() {
        if (items.empty()) {                 // READ on an empty inbox
            return;                          // nothing to remove
        }
        items[0] = std::move(items.back());  // move the last email into the root
        items.pop_back();                    // drop the now-empty last slot
        if (!items.empty()) {                // something is left to reorder
            siftDown(0);                     // sink the new root to its place
        }
    }

    // The highest-priority email, or nullptr if the heap is empty: O(1).
    const Email* peek() const {
        return items.empty() ? nullptr : &items[0];
    }

    // Number of unread emails: O(1).
    std::size_t size() const {
        return items.size();
    }
};

// ---------------------------------------------------------------------------
// Class EmailManager: turns each command line into heap operations and output.
// Source: combination. The command dispatch, the output formats, and the
// "blank line before every block except the first" approach came from Gemma.
// Trimming '\r'/spaces, validating EMAIL lines (field count, category, date),
// and parsing without a stringstream were authored by me.
// ---------------------------------------------------------------------------
class EmailManager {
private:
    MaxHeap inbox;          // the CEO's unread emails
    bool printedAnything;   // true once any output block has been printed
    int lineNumber;         // current line of the input, used in error messages

    // Remove trailing spaces, tabs and '\r' (from Windows line endings).
    static std::string trimRight(const std::string& s) {
        std::size_t end = s.find_last_not_of(" \t\r\n"); // last real character
        return end == std::string::npos ? "" : s.substr(0, end + 1);
    }

    // Remove leading and trailing spaces/tabs from one field.
    static std::string trim(const std::string& s) {
        std::size_t start = s.find_first_not_of(" \t");  // first real character
        if (start == std::string::npos) {                // the field is all blanks
            return "";
        }
        std::size_t end = s.find_last_not_of(" \t");     // last real character
        return s.substr(start, end - start + 1);
    }

    // Find a category's index in CATEGORY_NAMES, or -1 if it isn't one.
    static int categoryIndex(const std::string& name) {
        for (int i = 0; i < NUM_CATEGORIES; ++i) {       // check each known category
            if (name == CATEGORY_NAMES[i]) {
                return i;                                // found it
            }
        }
        return -1;                                       // not a valid category
    }

    // Turn MM-DD-YYYY into YYYYMMDD. Returns -1 if the text isn't a date.
    static int parseDate(const std::string& text) {
        if (text.size() != 10 || text[2] != '-' || text[5] != '-') { // wrong shape
            return -1;
        }
        for (std::size_t i = 0; i < text.size(); ++i) {  // every other character must be a digit
            if (i != 2 && i != 5 && (text[i] < '0' || text[i] > '9')) {
                return -1;
            }
        }
        int month = std::stoi(text.substr(0, 2));        // MM
        int day = std::stoi(text.substr(3, 2));          // DD
        int year = std::stoi(text.substr(6, 4));         // YYYY
        if (month < 1 || month > 12 || day < 1 || day > 31) { // out-of-range values
            return -1;
        }
        return year * 10000 + month * 100 + day;         // newer dates give bigger numbers
    }

    // Print a blank line before every output block except the first, so
    // blocks are separated without leaving an extra blank line at the end.
    void startBlock() {
        if (printedAnything) {      // something came before this block
            std::cout << '\n';      // separate the blocks
        }
        printedAnything = true;     // later blocks will need a separator
    }

    // Report a bad input line on stderr so normal output stays clean.
    void warn(const std::string& message) const {
        std::cerr << "Line " << lineNumber << ": " << message << '\n';
    }

    // Handle "EMAIL <category>,<subject>,<date>" (rest = text after "EMAIL ").
    void addEmail(const std::string& rest) {
        std::size_t firstComma = rest.find(',');                 // ends the category
        std::size_t lastComma = rest.rfind(',');                 // starts the date
        if (firstComma == std::string::npos || firstComma == lastComma) { // fewer than 3 fields
            warn("EMAIL needs <category>,<subject>,<date>");
            return;
        }
        std::string category = trim(rest.substr(0, firstComma)); // sender category text
        std::string subject = trim(rest.substr(firstComma + 1, lastComma - firstComma - 1)); // subject (inner spaces kept)
        std::string date = trim(rest.substr(lastComma + 1));     // date text

        int cat = categoryIndex(category);                       // position in the priority list
        if (cat < 0) {                                           // unknown sender category
            warn("unknown sender category '" + category + "'");
            return;
        }
        int dateNumber = parseDate(date);                        // YYYYMMDD value
        if (dateNumber < 0) {                                    // date isn't MM-DD-YYYY
            warn("date '" + date + "' is not MM-DD-YYYY");
            return;
        }
        inbox.push(Email(cat, subject, date, dateNumber));       // queue the email
    }

    // Handle NEXT: show the top email without removing it.
    void showNext() {
        const Email* top = inbox.peek();     // highest-priority email, if any
        if (top == nullptr) {                // empty inbox: print nothing
            return;
        }
        startBlock();                        // blank line between blocks
        std::cout << "Next email:\n"
                  << "\tSender: " << top->sender() << '\n'
                  << "\tSubject: " << top->subjectLine() << '\n'
                  << "\tDate: " << top->dateText() << '\n';
    }

    // Handle COUNT: show how many emails are still unread.
    void showCount() {
        startBlock();                        // blank line between blocks
        std::cout << "There are " << inbox.size() << " emails to read.\n";
    }

public:
    // Constructor: empty inbox, nothing printed yet, no lines read.
    EmailManager() : printedAnything(false), lineNumber(0) {}

    // Process one raw line from the test file.
    void processLine(const std::string& raw) {
        ++lineNumber;                                  // track position for warnings
        std::string line = trimRight(raw);             // drop '\r' and trailing spaces
        if (line.empty()) {                            // skip blank lines
            return;
        }
        if (line.compare(0, 6, "EMAIL ") == 0) {       // starts with "EMAIL "
            addEmail(line.substr(6));                  // the fields after the space
        } else if (line == "NEXT") {
            showNext();
        } else if (line == "READ") {
            inbox.pop();                               // remove top email without showing it
        } else if (line == "COUNT") {
            showCount();
        } else {
            warn("unknown command '" + line + "'");    // anything else is ignored
        }
    }

    // Process every line of a stream until it runs out.
    void processAll(std::istream& in) {
        std::string line;                              // one line of input
        while (std::getline(in, line)) {               // read until end of file
            processLine(line);
        }
    }
};

// ---------------------------------------------------------------------------
// main: choose the input (file argument or standard input) and run it.
// Source: combination. Gemma chose between a file argument and std::cin, but
// its ifstream was destroyed before it was read. Keeping the stream alive for
// the whole run and stopping with an error when the file can't be opened were
// authored by me.
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    EmailManager manager;                        // owns the inbox and the output
    if (argc > 1) {                              // a test file name was given
        std::ifstream file(argv[1]);             // lives until the end of this block
        if (!file.is_open()) {                   // the file couldn't be opened
            std::cerr << "Error: could not open " << argv[1] << '\n';
            return 1;                            // signal failure to the shell
        }
        manager.processAll(file);                // run every command in the file
    } else {
        manager.processAll(std::cin);            // no argument: read redirected input
    }
    return 0;                                    // finished normally
}
