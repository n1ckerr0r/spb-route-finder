#include <gtest/gtest.h>

#include "test_support.hpp"

TEST(DataPrinterTest, ConvertTime) {
    DataPrinter data_printer_test(json::object(), 0);

    EXPECT_EQ(DataPrinterTestAccess::convertTime(
                  data_printer_test, "2026-03-03T22:55:00+03:00"),
              "03.03.2026 22:55");
}
