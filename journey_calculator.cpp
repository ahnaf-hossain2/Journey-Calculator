#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <map>
#include <regex>
#include <stdexcept>
#include <limits>

// Constants class to centralize all conversion factors
class ConversionConstants {
public:
  static constexpr double KM_TO_M = 1000.0;
  static constexpr double MILES_TO_KM = 1.60934;
  static constexpr double M_TO_FT = 3.28084;
  static constexpr double FT_TO_YD = 1.0 / 3.0;
  static constexpr double SECONDS_IN_MIN = 60.0;
  static constexpr double MINUTES_IN_HOUR = 60.0;
  static constexpr double HOURS_IN_DAY = 24.0;
};

// Result structure to hold calculation outputs
struct CalculationResult {
  double primaryValue;
  std::map<std::string, double> equivalents;
  std::string primaryUnit;
};

class UnitConverter {
public:
  static double toMeters(double value, const std::string& unit) {
    static const std::map<std::string, double> conversions = {
        {"m", 1.0},
        {"km", ConversionConstants::KM_TO_M},
        {"mi", ConversionConstants::MILES_TO_KM * ConversionConstants::KM_TO_M},
        {"ft", 1.0 / ConversionConstants::M_TO_FT},
        {"yd", 1.0 / ConversionConstants::M_TO_FT * ConversionConstants::FT_TO_YD}};

    auto it = conversions.find(unit);
    if (it == conversions.end()) {
      throw std::invalid_argument("Unsupported distance unit: " + unit);
    }
    return value * it->second;
  }

  static double toSeconds(double value, const std::string& unit) {
    static const std::map<std::string, double> conversions = {
        {"s", 1.0},
        {"min", ConversionConstants::SECONDS_IN_MIN},
        {"h", ConversionConstants::SECONDS_IN_MIN * ConversionConstants::MINUTES_IN_HOUR},
        {"d", ConversionConstants::SECONDS_IN_MIN * ConversionConstants::MINUTES_IN_HOUR * ConversionConstants::HOURS_IN_DAY}};

    auto it = conversions.find(unit);
    if (it == conversions.end()) {
      throw std::invalid_argument("Unsupported time unit: " + unit);
    }
    return value * it->second;
  }

  static double toSpeed(double value, const std::string& unit) {
    static const std::map<std::string, double> conversions = {
        {"m/s", 1.0},
        {"km/h", ConversionConstants::KM_TO_M / (ConversionConstants::MINUTES_IN_HOUR * ConversionConstants::SECONDS_IN_MIN)},
        {"mph", ConversionConstants::MILES_TO_KM * ConversionConstants::KM_TO_M / (ConversionConstants::MINUTES_IN_HOUR * ConversionConstants::SECONDS_IN_MIN)},
        {"ft/s", 1.0 / ConversionConstants::M_TO_FT},
        {"km/s", ConversionConstants::KM_TO_M}};

    auto it = conversions.find(unit);
    if (it == conversions.end()) {
      throw std::invalid_argument("Unsupported speed unit: " + unit);
    }
    return value * it->second;
  }
};

class InputParser {
public:
  static std::pair<double, std::string> parseUnitValue(const std::string& input) {
    std::regex pattern("([0-9.]+)\\s*([a-zA-Z/]+)");
    std::smatch match;
    if (!std::regex_match(input, match, pattern)) {
      throw std::invalid_argument("Invalid input format. Expected format: value unit (e.g., 100 km)");
    }
    return {std::stod(match[1].str()), match[2].str()};
  }

  static double parseCompositeTime(const std::string& input) {
    std::regex pattern("([0-9.]+)\\s*([a-zA-Z]+)");
    std::sregex_iterator it(input.begin(), input.end(), pattern);
    std::sregex_iterator end;

    double totalSeconds = 0.0;
    if (it == end) {
      throw std::invalid_argument("Invalid time format. Expected format: value unit (e.g., 1h 30min)");
    }

    while (it != end) {
      double value = std::stod((*it)[1].str());
      std::string unit = (*it)[2].str();
      totalSeconds += UnitConverter::toSeconds(value, unit);
      ++it;
    }

    return totalSeconds;
  }
};

class JourneyCalculator {
public:
  static CalculationResult calculateSpeed(double distance, const std::string& distUnit, double timeSeconds) {
    if (timeSeconds <= 0) {
      throw std::invalid_argument("Time must be positive");
    }

    double distanceMeters = UnitConverter::toMeters(distance, distUnit);
    double speedMS = distanceMeters / timeSeconds;

    return {
        speedMS,
        {
            {"km/h", speedMS * 3.6},
            {"ft/s", speedMS * ConversionConstants::M_TO_FT},
            {"mph", speedMS / (ConversionConstants::MILES_TO_KM * ConversionConstants::KM_TO_M / (ConversionConstants::MINUTES_IN_HOUR * ConversionConstants::SECONDS_IN_MIN))},
        },
        "m/s"};
  }

  static CalculationResult calculateDistance(double speed, const std::string& speedUnit, double timeSeconds) {
    if (timeSeconds <= 0) {
      throw std::invalid_argument("Time must be positive");
    }

    double speedMS = UnitConverter::toSpeed(speed, speedUnit);
    double distanceMeters = speedMS * timeSeconds;

    return {
        distanceMeters,
        {
            {"km", distanceMeters / ConversionConstants::KM_TO_M},
            {"ft", distanceMeters * ConversionConstants::M_TO_FT},
            {"mi", distanceMeters / (ConversionConstants::MILES_TO_KM * ConversionConstants::KM_TO_M)},
        },
        "m"};
  }

  static CalculationResult calculateTime(double distance, const std::string& distUnit, double speed, const std::string& speedUnit) {
    if (speed <= 0) {
      throw std::invalid_argument("Speed must be positive");
    }

    double distanceMeters = UnitConverter::toMeters(distance, distUnit);
    double speedMS = UnitConverter::toSpeed(speed, speedUnit);
    double timeSeconds = distanceMeters / speedMS;

    return {
        timeSeconds,
        {
            {"min", timeSeconds / ConversionConstants::SECONDS_IN_MIN},
            {"h", timeSeconds / (ConversionConstants::SECONDS_IN_MIN * ConversionConstants::MINUTES_IN_HOUR)},
            {"d", timeSeconds / (ConversionConstants::SECONDS_IN_MIN * ConversionConstants::MINUTES_IN_HOUR * ConversionConstants::HOURS_IN_DAY)},
        },
        "s"};
  }
};

class UserInterface {
public:
  void run() {
    printWelcome();

    char continueCalculation;
    do {
      try {
        processCalculation();
      } catch (const std::exception& e) {
        std::cerr << "\n*** Error: " << e.what() << " ***\n\n";
      }

      std::cout << "\nDo you want to perform another calculation? (y/n): ";
      std::cin >> continueCalculation;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } while (tolower(continueCalculation) == 'y');

    std::cout << "\nThank you for using the Journey Metrics Calculator!\n\n";
  }

private:
  void printWelcome() {
    std::cout << "\n";
    std::cout << "=================================================================\n";
    std::cout << "|         Welcome to the Journey Metrics Calculator!             |\n";
    std::cout << "=================================================================\n";
    std::cout << "\n";
    std::cout << "Supported units:\n";
    std::cout << "- Distance: m (meters), km (kilometers), mi (miles), ft (feet), yd (yards)\n";
    std::cout << "- Time: s (seconds), min (minutes), h (hours), d (days)\n";
    std::cout << "- Speed: m/s (meters per second), km/h (kilometers per hour), mph (miles per hour),\n";
    std::cout << "        ft/s (feet per second), km/s (kilometers per second)\n\n";
  }

  void processCalculation() {
    std::cout << "Select calculation type:\n";
    std::cout << "1. Speed\n";
    std::cout << "2. Distance\n";
    std::cout << "3. Time\n";
    std::cout << "Choice: ";

    int choice;
    std::cin >> choice;
    while (std::cin.fail() || choice < 1 || choice > 3) {
      std::cin.clear(); // clear the error flag
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
      std::cout << "Invalid choice. Please enter 1, 2, or 3: ";
      std::cin >> choice;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    switch (choice) {
      case 1:
        calculateSpeed();
        break;
      case 2:
        calculateDistance();
        break;
      case 3:
        calculateTime();
        break;
      default:
        throw std::invalid_argument("Invalid choice");
    }
  }

  void calculateSpeed() {
    std::cout << "\n";
    std::cout << "Enter distance value and unit (e.g., 100 km): ";
    auto [distance, distUnit] = getUnitValueInput();

    std::cout << "Enter time value (e.g., 1h 20min): ";
    std::string timeInput;
    std::getline(std::cin, timeInput);
    double timeSeconds = InputParser::parseCompositeTime(timeInput);

    auto result = JourneyCalculator::calculateSpeed(distance, distUnit, timeSeconds);
    printResult(result);
  }

  void calculateDistance() {
    std::cout << "\n";
    std::cout << "Enter speed value and unit (e.g., 50 km/h): ";
    auto [speed, speedUnit] = getUnitValueInput();

    std::cout << "Enter time value (e.g., 1h 20min): ";
    std::string timeInput;
    std::getline(std::cin, timeInput);
    double timeSeconds = InputParser::parseCompositeTime(timeInput);

    auto result = JourneyCalculator::calculateDistance(speed, speedUnit, timeSeconds);
    printResult(result);
  }

  void calculateTime() {
    std::cout << "\n";
    std::cout << "Enter distance value and unit (e.g., 100 km): ";
    auto [distance, distUnit] = getUnitValueInput();

    std::cout << "Enter speed value and unit (e.g., 50 km/h): ";
    auto [speed, speedUnit] = getUnitValueInput();

    auto result = JourneyCalculator::calculateTime(distance, distUnit, speed, speedUnit);
    printResult(result);
  }

  std::pair<double, std::string> getUnitValueInput() {
    std::string input;
    std::getline(std::cin, input);
    return InputParser::parseUnitValue(input);
  }

  void printResult(const CalculationResult& result) {
    std::cout << "\n";
    std::cout << "=================================================================\n";
    std::cout << "| Result: " << std::fixed << std::setprecision(2) << result.primaryValue << " " << result.primaryUnit << " |\n";
    std::cout << "=================================================================\n";
    std::cout << "Equivalent Values:\n";
    for (const auto& [unit, value] : result.equivalents) {
      std::cout << "- " << value << " " << unit << "\n";
    }
    std::cout << "\n";
  }
};

int main() {
  UserInterface ui;
  ui.run();
  return 0;
}