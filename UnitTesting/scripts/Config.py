from typing import TypedDict, List


class Primitive(TypedDict):
    type: str
    min: str
    max: str
    use_epsilon: bool


class Config(TypedDict):
    primitives: List[Primitive]
    string_value: str
    class_int_value_first: str
    class_int_value_second: str
    class_string_value_first: str
    class_string_value_second: str


config: Config = {
    "primitives": [
        {"type": "uint32_t", "min": "0U", "max": "UINT_MAX", "use_epsilon": False},
        {"type": "int32_t", "min": "INT_MIN", "max": "INT_MAX", "use_epsilon": False},
        {"type": "uint64_t", "min": "0ULL", "max": "ULLONG_MAX", "use_epsilon": False},
        {"type": "int64_t", "min": "LLONG_MIN", "max": "LLONG_MAX", "use_epsilon": False},
        {"type": "float", "min": "1.175494351E-38f", "max": "3.402823466E+38f", "use_epsilon": True},
        {"type": "double", "min": "2.2250738585072014E-308", "max": "1.7976931348623158E+308", "use_epsilon": True},
    ],
    "string_value": "This is a test string!",
    "class_int_value_first": "42",
    "class_int_value_second": "1337",
    "class_string_value_first": "shoop da whoop",
    "class_string_value_second": "Ima chargin mah lazer!",
}
