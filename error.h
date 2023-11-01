#pragma once

#include <stdexcept>

enum struct ErrorType {
  StdIoError,
  UnexpectedEof,
  SizeTooLarge,
  EmptyInput,
  InvalidGzHeader,
  InvalidBlockType,
  BlockType0LenMismatch,
  InvalidCodeLengths,
  HuffmanDecoderCodeNotFound,
  DistanceTooMuch,
  EndOfBlockNotFound,
  ReadDynamicCodebook,
  ChecksumMismatch,
  SizeMismatch,
};

struct Error : public std::exception {
 public:
  explicit Error(ErrorType error_type) : error_type{error_type} {}

  const char* what() const noexcept override {
    switch (error_type) {
      case ErrorType::StdIoError:
        return "StdIoError";
      case ErrorType::UnexpectedEof:
        return "UnexpectedEof";
      case ErrorType::SizeTooLarge:
        return "SizeTooLarge";
      case ErrorType::EmptyInput:
        return "EmptyInput";
      case ErrorType::InvalidGzHeader:
        return "InvalidGzHeader";
      case ErrorType::InvalidBlockType:
        return "InvalidBlockType";
      case ErrorType::BlockType0LenMismatch:
        return "BlockType0LenMismatch";
      case ErrorType::InvalidCodeLengths:
        return "InvalidCodeLengths";
      case ErrorType::HuffmanDecoderCodeNotFound:
        return "HuffmanDecoderCodeNotFound";
      case ErrorType::DistanceTooMuch:
        return "DistanceTooMuch";
      case ErrorType::EndOfBlockNotFound:
        return "EndOfBlockNotFound";
      case ErrorType::ReadDynamicCodebook:
        return "ReadDynamicCodebook";
      case ErrorType::ChecksumMismatch:
        return "ChecksumMismatch";
      case ErrorType::SizeMismatch:
        return "SizeMismatch";
      default:
        return "Unknown Error";
    }
  }

 private:
  ErrorType error_type;
};
