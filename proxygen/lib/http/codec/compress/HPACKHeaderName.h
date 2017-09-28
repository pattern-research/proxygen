/*
 *  Copyright (c) 2017, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <iostream>
#include <boost/variant.hpp>
#include <proxygen/lib/http/HTTPCommonHeaders.h>

namespace proxygen {

/*
 * HPACKHeaderName stores the header name of HPACKHeaders. If
 * the header name is in HTTPCommonHeader, it will store a
 * pointer to the common header, otherwise, it will store a
 * pointer to a the dynamically allocated std::string
 */
class HPACKHeaderName {
 public:
  HPACKHeaderName() {}

  explicit HPACKHeaderName(folly::StringPiece name) {
    storeAddress(name);
  }
  HPACKHeaderName(const HPACKHeaderName& headerName) {
    copyAddress(headerName);
  }
  HPACKHeaderName(HPACKHeaderName&& goner) noexcept {
    moveAddress(goner);
  }
  void operator=(folly::StringPiece name) {
    resetAddress();
    storeAddress(name);
  }
  void operator=(const HPACKHeaderName& headerName) {
    resetAddress();
    copyAddress(headerName);
  }
  void operator=(HPACKHeaderName&& goner) noexcept {
    resetAddress();
    moveAddress(goner);
  }

  ~HPACKHeaderName() {
    resetAddress();
  }

  /*
   * Compare the strings stored in HPACKHeaderName
   */
  bool operator==(const HPACKHeaderName& headerName) const {
    return address_ == headerName.address_ ||
      *address_ == *(headerName.address_);
  }
  bool operator!=(const HPACKHeaderName& headerName) const {
    // Utilize the == overloaded operator
    return !(*this == headerName);
  }
  bool operator>(const HPACKHeaderName& headerName) const {
    if (!isAllocated() && !headerName.isAllocated()) {
      // Common header tables are aligned alphabetically (unit tested as well
      // to ensure it isn't accidentally changed)
      return address_ > headerName.address_;
    } else {
      return *address_ > *(headerName.address_);
    }
  }
  bool operator<(const HPACKHeaderName& headerName) const {
    if (!isAllocated() && !headerName.isAllocated()) {
      // Common header tables are aligned alphabetically (unit tested as well
      // to ensure it isn't accidentally changed)
      return address_ < headerName.address_;
    } else {
      return *address_ < *(headerName.address_);
    }
  }
  bool operator>=(const HPACKHeaderName& headerName) const {
    // Utilize existing < overloaded operator
    return !(*this < headerName);
  }
  bool operator<=(const HPACKHeaderName& headerName) const {
    // Utilize existing > overload operator
    return !(*this > headerName);
  }

  /*
   * Return std::string stored in HPACKHeaderName
   */
  const std::string& get() const {
    return *address_;
  }

  /*
   * Return whether or not address_ points to header
   * name in HTTPCommonHeaders
   */
  bool isCommonName() {
    return address_ != nullptr && !isAllocated();
  }

  /*
   * Directly call std::string member functions
   */
  uint32_t size() const {
    return (uint32_t)(address_->size());
  }
  const char* data() {
    return address_->data();
  }
  const char* c_str() const {
    return address_->c_str();
  }

 private:
  /*
   * Store a reference to either a common header or newly allocated string
   */
  void storeAddress(folly::StringPiece name) {
    HTTPHeaderCode headerCode = HTTPCommonHeaders::hash(
      name.data(), name.size());
    if (headerCode == HTTPHeaderCode::HTTP_HEADER_NONE ||
        headerCode == HTTPHeaderCode::HTTP_HEADER_OTHER) {
      std::string* newAddress = new std::string(name.size(), 0);
      std::transform(name.begin(), name.end(), newAddress->begin(), ::tolower);
      address_ = newAddress;
    } else {
      address_ = HTTPCommonHeaders::getPointerToHeaderName(
        headerCode, TABLE_LOWERCASE);
    }
  }

  /*
   * Copy the address_ from another HPACKHeaderName
   */
  void copyAddress(const HPACKHeaderName& headerName) {
    if (headerName.isAllocated()) {
      address_ = new std::string(headerName.get());
    } else {
      address_ = headerName.address_;
    }
  }

  /*
   * Move the address_ from another HPACKHeaderName
   */
  void moveAddress(HPACKHeaderName& goner) {
    address_ = goner.address_;
    goner.address_ = nullptr;
  }

  /*
   * Delete any allocated memory and reset address_ to nullptr
   */
  void resetAddress() {
    if (isAllocated()) {
      delete address_;
    }
    address_ = nullptr;
  }

  /*
   * Returns whether the underlying address_ points to a string that was
   * allocated (memory) by this instance
   */
  bool isAllocated() const {
    if (address_ == nullptr) {
      return false;
    } else {
      return !HTTPCommonHeaders::isHeaderNameFromTable(
        address_, TABLE_LOWERCASE);
    }
  }

  /*
   * Address either stores a pointer to a header name in HTTPCommonHeaders,
   * or stores a pointer to a dynamically allocated std::string
   */
  const std::string* address_ = nullptr;
};

inline std::ostream& operator<<(std::ostream& os, const HPACKHeaderName& name) {
  os << name.get();
  return os;
}

} // proxygen

namespace std {

template<>
struct hash<proxygen::HPACKHeaderName> {
  size_t operator()(const proxygen::HPACKHeaderName& headerName) const {
    return std::hash<std::string>()(headerName.get());
  }
};

} // std
