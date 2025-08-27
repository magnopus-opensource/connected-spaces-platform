import sys
import unittest
from word_reader import WordReader
import Parser
import MetadataTypes

class TestParserParseEnum(unittest.TestCase):
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


class TestParserParseType(unittest.TestCase):
    def setUp(self):
        self.parser = Parser.Parser(log_file=sys.stdout)
        self.maxDiff = None

    def __parse_type(self, type_str: str) -> MetadataTypes.TypeMetadata:
        wordreader = WordReader(type_str)
        word = wordreader.next_word()
        result, word = self.parser._Parser__parse_type(wordreader, word)

        self.assertIsNone(word, f"Expected to consume entire type string, but got leftover word '{word}'")
        return result

    def test_parse_primitive_type(self):
        """ Test parsing a simple type. """
        result = self.__parse_type("int")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_primitive = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_const_simple_type(self):
        """ Test parsing a simple type. """
        result = self.__parse_type("const int")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_const = True
        expected.is_primitive = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_namespaced_type(self):
        """ Test parsing a namespaced type. """
        result = self.__parse_type("eggs::and::bacon")

        expected = MetadataTypes.TypeMetadata("eggs::and", "bacon")
        expected.template_name = "bacon"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_pointer_type(self):
        """ Test parsing a pointer type. """
        result = self.__parse_type("int*")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_pointer = True
        expected.is_pointer_or_reference = True
        expected.is_primitive = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_reference_type(self):
        """ Test parsing a reference type. """
        result = self.__parse_type("int&")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_reference = True
        expected.is_pointer_or_reference = True
        expected.is_primitive = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_const_pointer_type(self):
        """ Test parsing a const pointer type. """
        result = self.__parse_type("const int*")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_const = True
        expected.is_pointer = True
        expected.is_pointer_or_reference = True
        expected.is_primitive = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_template_type(self):
        """ Test parsing a template type. """
        result = self.__parse_type("std::vector<int>")

        int_type = self.__parse_type("int")

        expected = MetadataTypes.TypeMetadata("std", "vector")
        expected.is_template = True
        expected.template_name = "vector"
        expected.template_arguments = [
            MetadataTypes.TemplateArgumentMetadata(
                type=int_type,
                is_last=True
            )
        ]

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_multi_template_type(self):
        """ Test parsing a template type with two arguments to the template. """
        result = self.__parse_type("std::map<int, std::string>")

        int_type = self.__parse_type("int")
        string_type = self.__parse_type("std::string")

        expected = MetadataTypes.TypeMetadata("std", "map")
        expected.is_template = True
        expected.template_name = "map"
        expected.template_arguments = [
            MetadataTypes.TemplateArgumentMetadata(
                type=int_type,
                is_last=False
            ),
            MetadataTypes.TemplateArgumentMetadata(
                type=string_type,
                is_last=True
            )
        ]

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_nested_template_type(self):
        """ Test parsing a template type with a nested template argument. """
        result = self.__parse_type("std::map<int, std::vector<std::string>>")

        int_type = self.__parse_type("int")
        vector_type = self.__parse_type("std::vector<std::string>")

        expected = MetadataTypes.TypeMetadata("std", "map")
        expected.is_template = True
        expected.template_name = "map"
        expected.template_arguments = [
            MetadataTypes.TemplateArgumentMetadata(
                type=int_type,
                is_last=False
            ),
            MetadataTypes.TemplateArgumentMetadata(
                type=vector_type,
                is_last=True
            )
        ]

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_forward_declared_type(self):
        """ Test parsing a forward-declared type. """
        result = self.__parse_type("class Foo")

        expected = MetadataTypes.TypeMetadata("", "Foo")
        expected.is_inline_forward = True
        expected.template_name = "Foo"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_optional_type(self):
        """ Test the special parsing of optional types (using csp::common::Optional). """
        result = self.__parse_type("csp::common::Optional<int>")

        expected = MetadataTypes.TypeMetadata("", "int")
        expected.is_const = False
        expected.is_primitive = True
        expected.is_optional = True
        expected.template_name = "int"
        expected.template_arguments = []

        self.assertDictEqual(result.__dict__, expected.__dict__)

    def test_parse_function_type(self):
        """ Test parsing a function type. """
        result = self.__parse_type("std::function<void()>")

        function_metadata = MetadataTypes.FunctionMetadata(None, 0, 0, None, None, None, False, False, [])

        expected = MetadataTypes.TypeMetadata("std", "function")
        expected.is_function_signature = True
        expected.function_signature = function_metadata
        expected.template_arguments = []
        expected.template_name = "function"

        self.assertDictEqual(result.__dict__, expected.__dict__)
