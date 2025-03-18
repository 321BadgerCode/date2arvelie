#include <iostream>
#include <cstring>
#include <algorithm>
#include <map>
#include <ctime>
#include <tuple>
#include <iomanip>
#include <stdexcept>

using namespace std;

#define EPOCH 2006
#define DEFAULT_CENTURY 1900
#define VERSION "1.0.0"

// Exception class that has line number and function name
class ExceptionWithLine : public exception {
public:
	ExceptionWithLine(const string& msg, const string& file, unsigned int line, const string& func) : msg(msg), file(file), line(line), func(func) {}
	const char* what() const noexcept override {
		return msg.c_str();
	}
	const string& get_file() const {
		return file;
	}
	int get_line() const {
		return line;
	}
	const string& get_func() const {
		return func;
	}
private:
	string msg;
	string file;
	unsigned int line;
	string func;
};

// Month mapping for full and abbreviated names
map<string, unsigned int> month_map = {
	{"jan", 1}, {"january", 1}, {"feb", 2}, {"february", 2},
	{"mar", 3}, {"march", 3}, {"apr", 4}, {"april", 4},
	{"may", 5}, {"jun", 6}, {"june", 6}, {"jul", 7}, {"july", 7},
	{"aug", 8}, {"august", 8}, {"sep", 9}, {"september", 9},
	{"oct", 10}, {"october", 10}, {"nov", 11}, {"november", 11},
	{"dec", 12}, {"december", 12}
};

// Convert standard date to Arvelie format
string to_arvelie(unsigned int year, unsigned int month, unsigned int day, unsigned int epoch=EPOCH) {
	// Calculate Arvelie year
	unsigned int arvelie_year = year - epoch;

	// Check if the year is a leap year
	bool is_leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

	// Array of cumulative days at the start of each month
	int cumulative_days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	// Adjust for leap year
	if (is_leap_year) {
		for (int i = 2; i < 12; ++i) {
			cumulative_days[i]++;
		}
	}

	// Compute the day of the year (DOY)
	unsigned int doy = cumulative_days[month - 1] + day;

	// Handle special cases for Year Day and Leap Day
	if (doy == 365 && !is_leap_year) {
		return to_string(arvelie_year) + "+00";
	}
	if (doy == 366 && is_leap_year) {
		return to_string(arvelie_year) + "+01";
	}

	// Calculate month index and day within the month
	const int DAYS_IN_ARVELIE_MONTH = 14;
	unsigned int month_index = (doy - 1) / DAYS_IN_ARVELIE_MONTH;
	unsigned int day_in_month = (doy - 1) % DAYS_IN_ARVELIE_MONTH;

	// Convert month index to corresponding letter ('A' to 'Z')
	char month_letter = 'A' + month_index;

	// Format and return Arvelie date
	return (arvelie_year % 100 < 10 ? "0" : "") + to_string(arvelie_year % 100) + month_letter + (day_in_month < 10 ? "0" : "") + to_string(day_in_month);
}

// Utility function to replace delimiters with a consistent separator (hyphen '-')
string normalize_date_format(const string& date_str) {
	string normalized_date = date_str;
	replace(normalized_date.begin(), normalized_date.end(), '/', '-'); // Replace slashes with hyphens
	replace(normalized_date.begin(), normalized_date.end(), '.', '-'); // Replace dots with hyphens
	return normalized_date;
}

// Check if the string is a number
bool is_number(const string &str) {
	return !str.empty() && all_of(str.begin(), str.end(), ::isdigit);
}

// Check if valid date parts are provided
bool is_valid_date(unsigned int year, unsigned int month, unsigned int day) {
	if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31) {
		return false;
	}
	return true;
}

// Date part enumeration
enum DatePart {
	Year,
	Month,
	Day
};

// Parse the date string based on the format
tuple<unsigned int, unsigned int, unsigned int> parse_date(const string& format, const string& date_str) {
	int year = -1, month = -1, day = -1;
	
	// Normalize the date string (handle slashes and dots)
	string normalized_date = normalize_date_format(date_str);

	// Handle named months and day parsing
	if (normalized_date.find(",") != string::npos) {
		istringstream ss(normalized_date);
		string month_str, day_str, year_str;
		ss >> month_str >> day_str >> year_str;

		// Trim the comma from the month name if present
		month_str.erase(remove(month_str.begin(), month_str.end(), ','), month_str.end());

		// Convert month to lowercase for lookup
		transform(month_str.begin(), month_str.end(), month_str.begin(), ::tolower);

		try {
			month = month_map[month_str];
			day = stoi(day_str);
			year = stoi(year_str);
		} catch (const exception& e) {
			throw ExceptionWithLine("Invalid date input when parsing date that uses the month naming format.", __FILE__, __LINE__, __func__);
		}
	} else {
		// Default handling for different formats: "ymd", "mdy", "dmy"
		istringstream date_ss(normalized_date);
		string date_parts[3];
		string segment;
		unsigned int idx = 0;

		// Split date string by '-'
		while (getline(date_ss, segment, '-')) {
			date_parts[idx++] = segment;
		}

		// Handle different formats (ymd, mdy, dmy)
		if (format == "ISO" || format == "ISO 8601") {
			year = stoi(date_parts[0]);
			month = stoi(date_parts[1]);
			day = stoi(date_parts[2]);
		} else if (format != "") {
			DatePart order[3] = {DatePart::Year, DatePart::Month, DatePart::Day};
			istringstream format_ss(format);
			string format_parts[3];
			idx = 0;
			if (format.find('-') == string::npos) {
				// Treat each segment as each character cuz format is "ymd" or "mdy" or "dmy"
				for (int i = 0; i < 3; i++) {
					format_parts[i] = format[i];
				}
			} else {
				// Split format string by '-'
				while (getline(format_ss, segment, '-')) {
					format_parts[idx++] = segment;
				}
			}
			for (int i = 0; i < 3; i++) {
				if (format_parts[i][0] == 'y') {
					order[i] = DatePart::Year;
				} else if (format_parts[i][0] == 'm') {
					order[i] = DatePart::Month;
				} else if (format_parts[i][0] == 'd') {
					order[i] = DatePart::Day;
				}
			}
			for (int i = 0; i < 3; i++) {
				if (order[i] == DatePart::Year) {
					year = stoi(date_parts[i]);
				} else if (order[i] == DatePart::Month) {
					month = stoi(date_parts[i]);
				} else if (order[i] == DatePart::Day) {
					day = stoi(date_parts[i]);
				}
			}
		} else {
			// Default assumption: try to detect format based on input structure
			DatePart order[3] = {DatePart::Day, DatePart::Year, DatePart::Month};

shuffle_order:
			// Shift the order from dym to ymd to mdy
			next_permutation(order, order + 3); // order = {order[1], order[2], order[0]};
			for (int i = 0; i < 3; i++) {
				if (order[i] == DatePart::Year) {
					year = stoi(date_parts[i]);
				} else if (order[i] == DatePart::Month) {
					month = stoi(date_parts[i]);
				} else if (order[i] == DatePart::Day) {
					day = stoi(date_parts[i]);
				}
			}
			if (!is_valid_date(year, month, day)) {
				if (*order == *new DatePart[3]{DatePart::Day, DatePart::Year, DatePart::Month}) {
					throw ExceptionWithLine("Invalid date input: The date format is not recognized or is invalid.", __FILE__, __LINE__, __func__);
				}
				goto shuffle_order;
			}
		}
	}

	// Validate the parsed date
	if (!is_valid_date(year, month, day))
		throw ExceptionWithLine("Invalid date input: The date values are out of range, making the date impossible and therefore invalid.", __FILE__, __LINE__, __func__);

	return make_tuple(year, month, day);
}

// Main function
int main(int argc, char* argv[]) {
	string format = "";
	unsigned int epoch = EPOCH;

	time_t now = time(0);
	tm *ltm = localtime(&now);
	unsigned int current_year = 1900 + ltm->tm_year;
	unsigned int current_month = 1 + ltm->tm_mon;
	unsigned int current_day = ltm->tm_mday;

	string date = to_string(current_year) + "-" + to_string(current_month) + "-" + to_string(current_day);

	for (int i = 1; i < argc; i++) {
		if (string(argv[i]) == "-h" || string(argv[i]) == "--help") {
			string mdy = to_string(current_month) + "/" + to_string(current_day) + "/" + to_string(current_year);
			string dmy = to_string(current_day) + "." + to_string(current_month) + "." + to_string(current_year);
			string name = "";
			for (auto const& [key, val] : month_map) {
				if (val == current_month) {
					name = key;
					break;
				}
			}
			name[0] = toupper(name[0]);

			cout << "Usage: date2arvelie [OPTIONS] DATE" << endl;
			cout << "Converts a standard date to Arvelie format." << endl;
			cout << "Options:" << endl;
			cout << "  -h, --help           Display this help message" << endl;
			cout << "  -f, --format FORMAT  Specify the input date format (ymd (ISO 8601), mdy, dmy)" << endl;
			cout << "  -e, --epoch YEAR     Specify the epoch year (default: 2006)" << endl;
			cout << "  -v, --version        Display the version of the program" << endl;
			cout << "Examples:" << endl;
			cout << "  date2arvelie " << date << endl;
			cout << "  date2arvelie -f mdy " << mdy << " -e " << current_year - 1 << endl;
			cout << "  date2arvelie --format dd-mm-yyyy " << dmy << endl;
			cout << "  date2arvelie \"" << name << " " << current_day << ", " << current_year << "\" --epoch " << current_year - 1 << endl;
			return 0;
		}
		else if (string(argv[i]) == "-f" || string(argv[i]) == "--format") {
			i++;
			if (i >= argc) {
				cerr << "Error: Missing argument for the format option." << endl;
				return 1;
			}
			format = argv[i];
		}
		else if (string(argv[i]) == "-e" || string(argv[i]) == "--epoch") {
			i++;
			if (i >= argc) {
				cerr << "Error: Missing argument for the epoch option." << endl;
				return 1;
			}
			if (!is_number(argv[i]) || stoi(argv[i]) < 0 || stoi(argv[i]) > current_year) {
				cerr << "Error: Invalid epoch year specified. Please provide a valid year." << endl;
				return 1;
			}
			epoch = stoi(argv[i]);
		}
		else if (string(argv[i]) == "-v" || string(argv[i]) == "--version") {
			cout << "Date to Arvelie Converter v" << VERSION << endl;
			return 0;
		}
		else {
			date = argv[i];
		}
	}

	try {
		unsigned int year, month, day;
		tie(year, month, day) = parse_date(format, date);
		string arvelie_date = to_arvelie(year, month, day, epoch);
		cout << "Arvelie Date: " << arvelie_date << endl;
	} catch (const ExceptionWithLine& e) {
		cout << "[._.]: The program encountered an error." << endl;
		cout << string(strlen(e.what()), '-') << endl;
		cerr << e.what() << endl;
		cerr << "In " << e.get_file() << " on line " << e.get_line() << " in function " << e.get_func() << "." << endl;
		cout << string(strlen(e.what()), '-') << endl;
		cout << "Please check and ensure that the command line arguments are valid and properly formatted." << endl;
		return 1;
	} catch (const exception& e) {
		cout << "[O-O]: The program encountered an error." << endl;
		cout << string(strlen(e.what()), '-') << endl;
		cerr << e.what() << endl;
		cout << string(strlen(e.what()), '-') << endl;
		cout << "Please check and ensure that the command line arguments are valid and properly formatted." << endl;
		return 1;
	}

	return 0;
}