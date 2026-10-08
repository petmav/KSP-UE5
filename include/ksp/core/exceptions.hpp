// C# exception types that kspcore throws where KSP's managed code would throw (README "Errors"). Index errors
// (IndexOutOfRangeException, List<T>'s ArgumentOutOfRangeException) are std::out_of_range; number parsing errors are
// ksp::dotnet::FormatException / OverflowException (ksp/dotnet/number.hpp).
#pragma once

#include <stdexcept>
#include <string>

namespace ksp {

// System.NullReferenceException
class NullReferenceException : public std::runtime_error {
public:
    NullReferenceException() : std::runtime_error("Object reference not set to an instance of an object") {}
    using std::runtime_error::runtime_error;
};

// System.ArgumentException (and ArgumentNullException, which derives from it in .NET)
class ArgumentException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};
class ArgumentNullException : public ArgumentException {
public:
    using ArgumentException::ArgumentException;
};

// System.InvalidCastException (e.g. unboxing an enum to int when its underlying type is not int)
class InvalidCastException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// System.DivideByZeroException (integer division / remainder by zero)
class DivideByZeroException : public std::runtime_error {
public:
    DivideByZeroException() : std::runtime_error("Attempted to divide by zero.") {}
    using std::runtime_error::runtime_error;
};

// System.Collections.Generic.KeyNotFoundException (Dictionary indexer with a missing key)
class KeyNotFoundException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// System.Exception thrown explicitly by KSP code (`throw new Exception("...")`)
class Exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace ksp
