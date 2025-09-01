import unittest
from word_reader import WordReader

class TestWordReader(unittest.TestCase):
    def test_next_word_basic(self):
        w = WordReader("Hello, world!")
        self.assertEqual(w.next_word(), "Hello")
        self.assertEqual(w.next_word(), ",")
        self.assertEqual(w.next_word(), "world!")
        self.assertIsNone(w.next_word())

    def test_skip_whitespace_and_peek_char(self):
        w = WordReader("   \t\n  Meow")
        w.skip_whitespace()
        self.assertEqual(w.peek_char(), "M")
        self.assertEqual(w.next_word(), "Meow")
        self.assertIsNone(w.next_word())

    def test_next_word_with_custom_delimiters(self):
        w = WordReader("a|b|c")
        self.assertEqual(w.next_word(delimiters={"|"}), "a")
        self.assertEqual(w.next_word(delimiters={"|"}), "|")
        self.assertEqual(w.next_word(delimiters={"|"}), "b")
        self.assertEqual(w.next_word(delimiters={"|"}), "|")
        self.assertEqual(w.next_word(delimiters={"|"}), "c")
        self.assertIsNone(w.next_word(delimiters={"|"}))

    def test_next_word_return_empty(self):
        w = WordReader("a,,b")
        self.assertEqual(w.next_word(delimiters={","}, return_empty=True), "a")
        self.assertEqual(w.next_word(delimiters={","}, return_empty=True), ",")
        self.assertEqual(w.next_word(delimiters={","}, return_empty=True), ",")
        self.assertEqual(w.next_word(delimiters={","}, return_empty=True), "b")
        self.assertIsNone(w.next_word(delimiters={","}, return_empty=True))

    def test_peek_char_and_skip_char(self):
        w = WordReader("abc")
        self.assertEqual(w.peek_char(), "a")
        w.skip_char()
        self.assertEqual(w.peek_char(), "b")
        w.skip_char(2)
        self.assertIsNone(w.peek_char())

    def test_skip(self):
        w = WordReader("a\nb\nc")
        w.skip(2)  # skips 'a' and '\n'
        self.assertEqual(w.current_line, 2)
        self.assertEqual(w.peek_char(), "b")
        w.skip(2)  # skips 'b' and '\n'
        self.assertEqual(w.current_line, 3)
        self.assertEqual(w.peek_char(), "c")

    def test_skip_line(self):
        w = WordReader("foo\nbar\nbaz")
        w.skip_line()
        self.assertEqual(w.peek_char(), "b")
        self.assertEqual(w.next_word(), "bar")
        # The head will be at the start of "baz" now
        # Skipping the line will move to the end
        w.skip_line()
        self.assertIsNone(w.peek_char())
        self.assertIsNone(w.next_word(), "baz")

    def test_find_next_of(self):
        w = WordReader("abc:def")
        w.skip(0)
        self.assertEqual(w.find_next_of({":"}), ":")
        w.skip(4)
        self.assertEqual(w.find_next_of({"e", "f"}), "e")

    def test_find_next_of_raises_index_error(self):
        w = WordReader("abc")
        with self.assertRaises(IndexError):
            w.find_next_of({"z"})

    def test_next_word_handles_only_whitespace(self):
        w = WordReader("   \t\n  ")
        self.assertIsNone(w.next_word())

    def test_next_word_handles_empty_string(self):
        w = WordReader("")
        self.assertIsNone(w.next_word())

    def test_skip_char_beyond_end(self):
        w = WordReader("abc")
        w.skip_char(10)
        self.assertIsNone(w.peek_char())

    def test_skip_beyond_end(self):
        w = WordReader("abc")
        w.skip(10)
        self.assertIsNone(w.peek_char())

    def test_skip_line_at_end(self):
        w = WordReader("abc")
        w.skip_line()
        self.assertIsNone(w.peek_char())


if __name__ == '__main__':
    unittest.main()