#pragma once
#include "vdb/protocol.hpp"

namespace VDP {

class Record : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using SizeT = uint32_t;
  explicit Record(std::string name);
  Record(std::string name, const std::vector<Part *> &fields);
  Record(std::string name, std::vector<PartPtr> fields);
  Record(std::string name, PacketReader &reader);

  void set_fields(std::vector<PartPtr> fields);
  std::vector<PartPtr> get_fields() const;

  void fetch() override;
  void read_data_from_message(PacketReader &reader) override;

  void Visit(Visitor *) const;

protected:
  // Encode the schema itself for transmission on the wire
  void write_schema(PacketWriter &sofar) const override;
  // Encode the data currently held according to schema for transmission on the
  // wire
  void write_message(PacketWriter &sofar) const override;

private:
  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

  std::vector<PartPtr> fields;
};

class String : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using FetchFunc = std::function<std::string()>;
  explicit String(
      std::string name, FetchFunc fetcher = []() { return "no value"; });
  void fetch() override;
  void set_value(std::string new_value);
  std::string get_value() const;

  void read_data_from_message(PacketReader &reader) override;

  void pprint(std::stringstream &ss, size_t indent) const override;
  void pprint_data(std::stringstream &ss, size_t indent) const override;

  void Visit(Visitor *) const;

protected:
  void write_schema(PacketWriter &sofar) const override;
  void write_message(PacketWriter &sofar) const override;

private:
  FetchFunc fetcher;
  std::string value;
};

// Template to reduce boiler plate for Schema wrappers for simple types
// Fixed size, numeric types  such as uin8_t, uint32, float, double
template <typename NumT, Type schemaType> class Number : public Part {
  friend PacketReader;
  friend PacketWriter;

public:
  using NumberType = NumT;
  static constexpr Type SchemaType = schemaType;

  // Checks to make sure this isn't misused
  static_assert(std::is_floating_point<NumberType>::value ||
                    std::is_integral<NumberType>::value,
                "Number type this is instantiated with must be floating point "
                "or integral");

  using FetchFunc = std::function<NumberType()>;
  explicit Number(
      std::string field_name,
      FetchFunc fetcher = []() { return (NumberType)0; })
      : Part(field_name), fetcher(fetcher) {}

  void fetch() override { value = fetcher(); }
  void set_value(NumberType val) { this->value = val; }
  NumberType get_value() const { return this->value; }

  void pprint(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t" << to_string(SchemaType);
  }
  void pprint_data(std::stringstream &ss, size_t indent) const override {
    add_indents(ss, indent);
    ss << name << ":\t";
    if (sizeof(NumberType) == 1) {
      ss << (int)value; // Otherwise, stringstream interprets uint8 as char and
                        // prints a char
    } else {
      ss << value;
    }
  }
  void read_data_from_message(PacketReader &reader) override {
    value = reader.get_number<NumberType>();
  }

protected:
  void write_schema(PacketWriter &sofar) const override {
    sofar.write_type(SchemaType); // Type
    sofar.write_string(name);     // Name
  }
  void write_message(PacketWriter &sofar) const override {
    sofar.write_number<NumberType>(value);
  }

private:
  FetchFunc fetcher;
  NumberType value = (NumberType)0;
};

class Float : public Number<float, Type::Float> {
public:
  using NumT = Number<float, Type::Float>;
  Float(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Double : public Number<double, Type::Double> {
public:
  using NumT = Number<double, Type::Double>;
  Double(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};

class Uint8 : public Number<uint8_t, Type::Uint8> {
public:
  using NumT = Number<uint8_t, Type::Uint8>;
  Uint8(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Uint16 : public Number<uint16_t, Type::Uint16> {
public:
  using NumT = Number<uint16_t, Type::Uint16>;
  Uint16(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Uint32 : public Number<uint32_t, Type::Uint32> {
public:
  using NumT = Number<uint32_t, Type::Uint32>;
  Uint32(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Uint64 : public Number<uint64_t, Type::Uint64> {
public:
  using NumT = Number<uint64_t, Type::Uint64>;
  Uint64(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};

class Int8 : public Number<int8_t, Type::Int8> {
public:
  using NumT = Number<int8_t, Type::Int8>;
  Int8(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Int16 : public Number<int16_t, Type::Int16> {
public:
  using NumT = Number<int16_t, Type::Int16>;
  Int16(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};
class Int32 : public Number<int32_t, Type::Int32> {
public:
  using NumT = Number<int32_t, Type::Int32>;
  Int32(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};

class Int64 : public Number<int64_t, Type::Int64> {
public:
  using NumT = Number<int64_t, Type::Int64>;
  Int64(
      std::string name,
      NumT::FetchFunc func = []() { return (NumT::NumberType)0; });
  void Visit(Visitor *) const;
};

class Visitor {
public:
  virtual void VisitRecord(const Record *) = 0;

  virtual void VisitString(const String *) = 0;

  virtual void VisitFloat(const Float *) = 0;
  virtual void VisitDouble(const Double *) = 0;

  virtual void VisitUint8(const Uint8 *) = 0;
  virtual void VisitUint16(const Uint16 *) = 0;
  virtual void VisitUint32(const Uint32 *) = 0;
  virtual void VisitUint64(const Uint64 *) = 0;

  virtual void VisitInt8(const Int8 *) = 0;
  virtual void VisitInt16(const Int16 *) = 0;
  virtual void VisitInt32(const Int32 *) = 0;
  virtual void VisitInt64(const Int64 *) = 0;
};

class UpcastNumbersVisitor : public Visitor {
public:
  virtual void VisitAnyFloat(const std::string &name, double value,
                             const Part *) = 0;
  virtual void VisitAnyInt(const std::string &name, int64_t value,
                           const Part *) = 0;
  virtual void VisitAnyUint(const std::string &name, uint64_t value,
                            const Part *) = 0;

  // Implemented to call Visitor::VisitAnyFloat
  void VisitFloat(const Float *) override;
  void VisitDouble(const Double *) override;

  // Implemented to call Visitor::VisitAnyUint
  void VisitUint8(const Uint8 *) override;
  void VisitUint16(const Uint16 *) override;
  void VisitUint32(const Uint32 *) override;
  void VisitUint64(const Uint64 *) override;

  // Implemented to call Visitor::VisitAnyInt
  void VisitInt8(const Int8 *) override;
  void VisitInt16(const Int16 *) override;
  void VisitInt32(const Int32 *) override;
  void VisitInt64(const Int64 *) override;
};

} // namespace VDP
