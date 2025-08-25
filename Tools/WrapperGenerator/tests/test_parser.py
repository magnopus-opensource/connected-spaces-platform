import sys
import unittest
from word_reader import WordReader
import Parser
import MetadataTypes

class TestParser(unittest.TestCase):
    def setUp(self):
        self.parser = Parser.Parser(log_file=sys.stdout)

    def test_parse_empty_enum(self):
        """ Test parsing an empty enum. """
        wordreader = WordReader(
        """
        enum Foo {}
        """)
        wordreader.next_word()  # 'enum'

        result = self.parser._Parser__parse_enum("test.h", wordreader)

        self.assertEqual(result.namespace, "")
        self.assertEqual(result.name, "Foo")
        self.assertEqual(result.full_safe_type_name, "_Foo")
        self.assertEqual(result.fields, [])
        self.assertEqual(result.is_flags, False)
        self.assertEqual(result.is_nested_type, False)
        self.assertEqual(result.doc_comments, None)

    def test_parse_enum_fields(self):
        """ Test parsing an enum with fields. """
        wordreader = WordReader(
        """
        enum EnumName {
            Foo,
            Bar,
            Baz
        }
        """)
        wordreader.next_word()  # 'enum'
        
        result = self.parser._Parser__parse_enum("test.h", wordreader)

        self.assertEqual(result.namespace, "")
        self.assertEqual(result.name, "EnumName")
        self.assertEqual(result.full_safe_type_name, "_EnumName")
        self.assertEqual(result.fields, [
            MetadataTypes.EnumFieldMetadata("Foo", None, None),
            MetadataTypes.EnumFieldMetadata("Bar", None, None),
            MetadataTypes.EnumFieldMetadata("Baz", None, None)
        ])
        self.assertEqual(result.is_flags, False)
        self.assertEqual(result.is_nested_type, False)
        self.assertEqual(result.doc_comments, None)

    def test_parse_enum_field_values(self):
        """ Test parsing an enum with fields that have explicit values. """
        wordreader = WordReader(
        """
        enum EnumName {
            Foo = -1,
            Bar = 0,
            Baz = 1234
        }
        """)
        wordreader.next_word()  # 'enum'
        
        result = self.parser._Parser__parse_enum("test.h", wordreader)

        self.assertEqual(result.namespace, "")
        self.assertEqual(result.name, "EnumName")
        self.assertEqual(result.full_safe_type_name, "_EnumName")
        self.assertEqual(result.fields, [
            # Note that values are stored as strings
            MetadataTypes.EnumFieldMetadata("Foo", "-1", None),
            MetadataTypes.EnumFieldMetadata("Bar", "0", None),
            MetadataTypes.EnumFieldMetadata("Baz", "1234", None)
        ])
        self.assertEqual(result.is_flags, False)
        self.assertEqual(result.is_nested_type, False)
        self.assertEqual(result.doc_comments, None)
