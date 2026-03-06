#pragma once
#include <gtest/gtest.h>
#include "core/db.h"
#include <cstdio>

class DBTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto& db = codelab::core::Database::GetInstance();
    db.Disconnect();
    std::remove("test_db.sqlite");
    db.Connect("test_db.sqlite");
    db.ApplySchema("db/schema.sql");
  }

  void TearDown() override {}
};