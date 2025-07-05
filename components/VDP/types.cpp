#include "vdb/types.hpp"
namespace VDP {

Record::Record(std::string name, const std::vector<Part *> &parts)
    : Part(std::move(name)), fields() {
  fields.reserve(parts.size());
  for (Part *f : parts) {
    fields.emplace_back(f);
  }
}
Record::Record(std::string name) : Part(std::move(name)), fields({}) {}
void Record::set_fields(std::vector<PartPtr> fs) { fields = std::move(fs); }
std::vector<PartPtr> Record::get_fields() const { return fields; }

Record::Record(std::string name, std::vector<PartPtr> parts)
    : Part(std::move(name)), fields(std::move(parts)) {}

Record::Record(std::string name, PacketReader &reader)
    : Part(std::move(name)), fields() {
  // Name and type already read, only need to read number of fields before child
  // data shows up
  const uint32_t size = reader.get_number<SizeT>();
  fields.reserve(size);
  for (size_t i = 0; i < size; i++) {
    fields.push_back(make_decoder(reader));
  }
}
void Record::fetch() {
  for (auto &field : fields) {
    field->fetch();
  }
}
void Record::write_schema(PacketWriter &sofar) const {
  sofar.write_type(Type::Record);           // Type
  sofar.write_string(name);                 // Name
  sofar.write_number<SizeT>(fields.size()); // Number of fields
  for (const PartPtr &field : fields) {
    field->write_schema(sofar);
  }
}
void Record::write_message(PacketWriter &sofar) const {
  for (auto &f : fields) {
    f->write_message(sofar);
  }
}

void Record::read_data_from_message(PacketReader &reader) {
  for (auto &f : fields) {
    f->read_data_from_message(reader);
  }
}

void String::pprint(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": string";
}
void String::pprint_data(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ":\t" << value;
}

void Record::pprint(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": record[" << fields.size() << "]{\n";
  for (const auto &f : fields) {

    f->pprint(ss, indent + 1);
    ss << '\n';
  }
  add_indents(ss, indent);
  ss << "}\n";
}
void Record::pprint_data(std::stringstream &ss, size_t indent) const {
  add_indents(ss, indent);
  ss << name << ": record[" << fields.size() << "]{\n";
  for (const auto &f : fields) {

    f->pprint_data(ss, indent + 1);
    ss << '\n';
  }
  add_indents(ss, indent);
  ss << "}\n";
}

String::String(std::string field_name, std::function<std::string()> fetcher)
    : Part(std::move(field_name)), fetcher(std::move(fetcher)) {}

void String::fetch() { value = fetcher(); }

void String::set_value(std::string new_value) { value = std::move(new_value); }

std::string String::get_value() const { return value; }

void String::write_schema(PacketWriter &sofar) const {
  sofar.write_type(Type::String); // Type
  sofar.write_string(name);       // Name
}

void String::write_message(PacketWriter &sofar) const {
  sofar.write_string(value);
}

void String::read_data_from_message(PacketReader &reader) {
  value = reader.get_string();
}

Float::Float(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Double::Double(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Uint8::Uint8(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Uint16::Uint16(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Uint32::Uint32(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Uint64::Uint64(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Int8::Int8(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Int16::Int16(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Int32::Int32(std::string name, NumT::FetchFunc func) : NumT(name, func) {}
Int64::Int64(std::string name, NumT::FetchFunc func) : NumT(name, func) {}

void Record::Visit(Visitor *v) { v->VisitRecord(this); }
void String::Visit(Visitor *v) { v->VisitString(this); }

void Float::Visit(Visitor *v) { v->VisitFloat(this); }
void Double::Visit(Visitor *v) { v->VisitDouble(this); }

void Uint8::Visit(Visitor *v) { v->VisitUint8(this); }
void Uint16::Visit(Visitor *v) { v->VisitUint16(this); }
void Uint32::Visit(Visitor *v) { v->VisitUint32(this); }
void Uint64::Visit(Visitor *v) { v->VisitUint64(this); }

void Int8::Visit(Visitor *v) { v->VisitInt8(this); }
void Int16::Visit(Visitor *v) { v->VisitInt16(this); }
void Int32::Visit(Visitor *v) { v->VisitInt32(this); }
void Int64::Visit(Visitor *v) { v->VisitInt64(this); }

void UpcastNumbersVisitor::VisitFloat(Float *f) {
  VisitAnyFloat(f->get_name(), f->get_value(), f);
}
void UpcastNumbersVisitor::VisitDouble(Double *f) {
  VisitAnyFloat(f->get_name(), f->get_value(), f);
}
void UpcastNumbersVisitor::VisitUint8(Uint8 *f) {
  VisitAnyUint(f->get_name(), (uint64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitUint16(Uint16 *f) {
  VisitAnyUint(f->get_name(), (uint64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitUint32(Uint32 *f) {
  VisitAnyUint(f->get_name(), (uint64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitUint64(Uint64 *f) {
  VisitAnyUint(f->get_name(), (uint64_t)f->get_value(), f);
}

void UpcastNumbersVisitor::VisitInt8(Int8 *f) {
  VisitAnyUint(f->get_name(), (int64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitInt16(Int16 *f) {
  VisitAnyUint(f->get_name(), (int64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitInt32(Int32 *f) {
  VisitAnyUint(f->get_name(), (int64_t)f->get_value(), f);
}
void UpcastNumbersVisitor::VisitInt64(Int64 *f) {
  VisitAnyUint(f->get_name(), (int64_t)f->get_value(), f);
}

} // namespace VDP
