#include "lte/input_parser.hpp"

#include <algorithm>
#include <charconv>
#include <iterator>
#include <string>
#include <system_error>

namespace lte {
namespace {

constexpr unsigned char ESC = 0x1b;

bool is_printable_ascii(const unsigned char ch)
{
    return ch >= 0x20 && ch <= 0x7e;
}

std::string bytes_to_string(const std::span<const unsigned char> bytes)
{
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

int decode_csi_modifier(const int value)
{
    if (value <= 1) {
        return 0;
    }

    const int encoded = value - 1;
    int modifiers = 0;
    if ((encoded & 1) != 0) {
        modifiers |= Shift;
    }
    if ((encoded & 2) != 0) {
        modifiers |= Alt;
    }
    if ((encoded & 4) != 0) {
        modifiers |= Ctrl;
    }
    if ((encoded & 8) != 0) {
        modifiers |= Meta;
    }
    return modifiers;
}

bool parse_int(const std::string_view input, int& out)
{
    if (input.empty()) {
        return false;
    }
    const auto first = input.data();
    const auto last = input.data() + input.size();
    auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc{} && ptr == last;
}

std::vector<int> parse_csi_params(std::string_view body)
{
    std::vector<int> params;
    std::size_t start = 0;
    while (start <= body.size()) {
        std::size_t end = body.find(';', start);
        if (end == std::string_view::npos) {
            end = body.size();
        }
        if (int value = 0; parse_int(body.substr(start, end - start), value)) {
            params.push_back(value);
        } else {
            params.push_back(0);
        }
        if (end == body.size()) {
            break;
        }
        start = end + 1;
    }
    return params;
}

int utf8_sequence_length(unsigned char lead)
{
    if ((lead & 0b1000'0000) == 0) {
        return 1;
    }
    if ((lead & 0b1110'0000) == 0b1100'0000) {
        return 2;
    }
    if ((lead & 0b1111'0000) == 0b1110'0000) {
        return 3;
    }
    if ((lead & 0b1111'1000) == 0b1111'0000) {
        return 4;
    }
    return 0;
}

bool valid_utf8_tail(std::span<const unsigned char> bytes)
{
    return std::all_of(bytes.begin(), bytes.end(), [](const unsigned char ch) {
        return (ch & 0b1100'0000) == 0b1000'0000;
    });
}

} // namespace

void InputParser::feed(std::span<const unsigned char> bytes)
{
    pending_.insert(pending_.end(), bytes.begin(), bytes.end());
}

ParseStatus InputParser::next_event(NativeEvent& out)
{
    if (pending_.empty()) {
        return ParseStatus::Empty;
    }

    if (pending_[0] == ESC) {
        return parse_escape(out);
    }

    return parse_utf8_or_ascii(out);
}

ParseStatus InputParser::flush_timeout(NativeEvent& out)
{
    if (pending_.size() == 1 && pending_[0] == ESC) {
        consume(1);
        out = make_key(KeyCode::Escape);
        return ParseStatus::Emit;
    }
    return next_event(out);
}

bool InputParser::empty() const
{
    return pending_.empty();
}

ParseStatus InputParser::parse_escape(NativeEvent& out)
{
    if (pending_.size() == 1) {
        return ParseStatus::NeedMore;
    }

    if (pending_[1] == '[') {
        return parse_csi(out);
    }

    if (pending_[1] == 'O') {
        return parse_ss3(out);
    }

    if (is_printable_ascii(pending_[1])) {
        const std::string text(1, static_cast<char>(pending_[1]));
        consume(2);
        out = make_key(KeyCode::Character, text, Modifier::Alt);
        return ParseStatus::Emit;
    }

    const std::string unknown = bytes_to_string(std::span(pending_.data(), std::min<std::size_t>(2, pending_.size())));
    consume(unknown.size());
    out = make_unknown(unknown);
    return ParseStatus::Emit;
}

ParseStatus InputParser::parse_csi(NativeEvent& out)
{
    if (pending_.size() < 3) {
        return ParseStatus::NeedMore;
    }

    if (const unsigned char final = pending_[2]; final == 'A' || final == 'B' || final == 'C' || final == 'D' || final == 'H' || final == 'F') {
        consume(3);
        switch (final) {
        case 'A':
            out = make_key(KeyCode::Up);
            return ParseStatus::Emit;
        case 'B':
            out = make_key(KeyCode::Down);
            return ParseStatus::Emit;
        case 'C':
            out = make_key(KeyCode::Right);
            return ParseStatus::Emit;
        case 'D':
            out = make_key(KeyCode::Left);
            return ParseStatus::Emit;
        case 'H':
            out = make_key(KeyCode::Home);
            return ParseStatus::Emit;
        case 'F':
            out = make_key(KeyCode::End);
            return ParseStatus::Emit;
        default:
            break;
        }
    }

    std::size_t final_pos = 2;
    while (final_pos < pending_.size()) {
        if (const unsigned char ch = pending_[final_pos]; ch >= 0x40 && ch <= 0x7e) {
            break;
        }
        ++final_pos;
    }

    if (final_pos == pending_.size()) {
        return ParseStatus::NeedMore;
    }

    const std::string body = bytes_to_string(std::span(pending_.data() + 2, final_pos - 2));
    const char final_char = static_cast<char>(pending_[final_pos]);
    const std::size_t sequence_len = final_pos + 1;

    if (body == "200" && final_char == '~') {
        constexpr unsigned char end_seq[] = {ESC, '[', '2', '0', '1', '~'};
        const auto begin = pending_.begin() + static_cast<std::ptrdiff_t>(sequence_len);
        const auto paste_end = std::search(begin, pending_.end(), std::begin(end_seq), std::end(end_seq));
        if (paste_end == pending_.end()) {
            return ParseStatus::NeedMore;
        }
        std::string text(reinterpret_cast<const char*>(&*begin), static_cast<std::size_t>(paste_end - begin));
        consume(paste_end - pending_.begin() + std::size(end_seq));
        out = make_paste(std::move(text));
        return ParseStatus::Emit;
    }

    const auto params = parse_csi_params(body);
    const int first = params.empty() ? 0 : params[0];
    const int modifiers = params.size() >= 2 ? decode_csi_modifier(params[1]) : 0;

    if (final_char == '~') {
        consume(sequence_len);
        switch (first) {
        case 1:
        case 7:
            out = make_key(KeyCode::Home, {}, modifiers);
            return ParseStatus::Emit;
        case 2:
            out = make_key(KeyCode::Insert, {}, modifiers);
            return ParseStatus::Emit;
        case 3:
            out = make_key(KeyCode::Delete, {}, modifiers);
            return ParseStatus::Emit;
        case 4:
        case 8:
            out = make_key(KeyCode::End, {}, modifiers);
            return ParseStatus::Emit;
        case 5:
            out = make_key(KeyCode::PageUp, {}, modifiers);
            return ParseStatus::Emit;
        case 6:
            out = make_key(KeyCode::PageDown, {}, modifiers);
            return ParseStatus::Emit;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 23:
        case 24: {
            int function_number = 0;
            if (first >= 11 && first <= 15) {
                function_number = first - 10;
            } else if (first >= 17 && first <= 21) {
                function_number = first - 11;
            } else {
                function_number = first - 12;
            }
            out = make_key(KeyCode::Function, {}, modifiers, function_number);
            return ParseStatus::Emit;
        }
        default:
            out = make_unknown(bytes_to_string(std::span(pending_.data(), sequence_len)));
            return ParseStatus::Emit;
        }
    }

    if (final_char == 'A' || final_char == 'B' || final_char == 'C' || final_char == 'D' ||
        final_char == 'H' || final_char == 'F') {
        consume(sequence_len);
        switch (final_char) {
        case 'A':
            out = make_key(KeyCode::Up, {}, modifiers);
            return ParseStatus::Emit;
        case 'B':
            out = make_key(KeyCode::Down, {}, modifiers);
            return ParseStatus::Emit;
        case 'C':
            out = make_key(KeyCode::Right, {}, modifiers);
            return ParseStatus::Emit;
        case 'D':
            out = make_key(KeyCode::Left, {}, modifiers);
            return ParseStatus::Emit;
        case 'H':
            out = make_key(KeyCode::Home, {}, modifiers);
            return ParseStatus::Emit;
        case 'F':
            out = make_key(KeyCode::End, {}, modifiers);
            return ParseStatus::Emit;
        default:
            ;
        }
    }

    out = make_unknown(bytes_to_string(std::span(pending_.data(), sequence_len)));
    consume(sequence_len);
    return ParseStatus::Emit;
}

ParseStatus InputParser::parse_ss3(NativeEvent& out)
{
    if (pending_.size() < 3) {
        return ParseStatus::NeedMore;
    }

    const char final = static_cast<char>(pending_[2]);
    consume(3);
    switch (final) {
    case 'A':
        out = make_key(KeyCode::Up);
        return ParseStatus::Emit;
    case 'B':
        out = make_key(KeyCode::Down);
        return ParseStatus::Emit;
    case 'C':
        out = make_key(KeyCode::Right);
        return ParseStatus::Emit;
    case 'D':
        out = make_key(KeyCode::Left);
        return ParseStatus::Emit;
    case 'H':
        out = make_key(KeyCode::Home);
        return ParseStatus::Emit;
    case 'F':
        out = make_key(KeyCode::End);
        return ParseStatus::Emit;
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
        out = make_key(KeyCode::Function, {}, 0, final - 'P' + 1);
        return ParseStatus::Emit;
    default:
        out = make_unknown(std::string("\x1bO", 2) + final);
        return ParseStatus::Emit;
    }
}

ParseStatus InputParser::parse_utf8_or_ascii(NativeEvent& out)
{
    const unsigned char ch = pending_[0];

    if (ch == '\r' || ch == '\n') {
        consume(1);
        out = make_key(KeyCode::Enter);
        return ParseStatus::Emit;
    }
    if (ch == '\t') {
        consume(1);
        out = make_key(KeyCode::Tab);
        return ParseStatus::Emit;
    }
    if (ch == 0x7f || ch == 0x08) {
        consume(1);
        out = make_key(KeyCode::Backspace);
        return ParseStatus::Emit;
    }

    if (ch >= 1 && ch <= 26) {
        const char letter = static_cast<char>('a' + ch - 1);
        consume(1);
        out = make_key(KeyCode::Character, std::string(1, letter), Modifier::Ctrl);
        return ParseStatus::Emit;
    }

    if (is_printable_ascii(ch)) {
        consume(1);
        out = make_key(KeyCode::Character, std::string(1, static_cast<char>(ch)));
        return ParseStatus::Emit;
    }

    const int length = utf8_sequence_length(ch);
    if (length == 0) {
        consume(1);
        out = make_unknown(std::string(1, static_cast<char>(ch)));
        return ParseStatus::Invalid;
    }
    if (pending_.size() < static_cast<std::size_t>(length)) {
        return ParseStatus::NeedMore;
    }

    const auto bytes = std::span(pending_.data(), static_cast<std::size_t>(length));
    if (!valid_utf8_tail(bytes.subspan(1))) {
        consume(1);
        out = make_unknown(std::string(1, static_cast<char>(ch)));
        return ParseStatus::Invalid;
    }

    std::string text = bytes_to_string(bytes);
    consume(static_cast<std::size_t>(length));
    out = make_key(KeyCode::Character, std::move(text));
    return ParseStatus::Emit;
}

void InputParser::consume(std::size_t count)
{
    pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(count));
}

} // namespace lte
