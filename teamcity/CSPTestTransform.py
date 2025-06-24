import argparse
import xml.etree.ElementTree as ET
import os, re

# Utility function for indenting the xml before writing out
def indent(elem, level=0):
    i = "\n" + level * "  "  # 2 spaces per level
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        for child in elem:
            indent(child, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def split_camel_case(name):
    # Insert space before each capital letter that's preceded by a lowercase or number
    return re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ', name)

def rename_attribute(element, old_name, new_name):
    if old_name in element.attrib:
        element.set(new_name, element.get(old_name))
        del element.attrib[old_name]

def process_junit_xml(input_path, output_path):
    tree = ET.parse(input_path)
    # We expect the route element to be <testsuites>
    testsuites_root = tree.getroot()

    # We express disabled tests as 'skipped'
    rename_attribute(testsuites_root, "disabled", "skipped")

    testsuites = testsuites_root.findall("testsuite")
    for suite in testsuites:
        rename_attribute(suite, "disabled", "skipped")
        
        # We inject the test suite name into each test case name, 
        # separated by carets, for readability with our test reporting platform.
        test_suite_prefix = suite.get("name")
        test_suite_prefix = test_suite_prefix.replace(".", " > ")

        for case in suite.findall("testcase"):
        
            # Test case name
            name = case.get("name", "")
            name = test_suite_prefix + " > " + split_camel_case(name)
            case.set("name", name)

            # Test case description
            if case.find("system-out") is None:
                # CSP currently doesn't write test descriptions to their test XML
                ET.SubElement(case, "system-out").text = "Placeholder test description."

            # Ignored test case
            status_attribute = case.get("status")
            if status_attribute is not None:
                if status_attribute == "notrun":
                    case.set("status", "Success")
                    ET.SubElement(case, "skipped")
                if status_attribute == "run":
                    case.set("status", "Success")
            else:
                raise Exception("Encountered a test case without a status attribute.")

            # Test case failures
            # We munge all failures into one single failure case, as our test platform only handles the one
            failures = case.findall("failure")
            failure_text = ""
            for failure in failures:
                case.set("status", "Fail")
                failure_text = failure_text + failure.get("message") + "\n\n"
                case.remove(failure)
            
            if failure_text != "":
                ET.SubElement(case, "failure", attrib={"message": failure_text})

    indent(testsuites_root)
    tree.write(output_path, encoding="utf-8", xml_declaration=True)

def main():
    parser = argparse.ArgumentParser(description="Transform JUnit XML reports.")
    parser.add_argument("-input", help="Path to input JUnit XML file")
    parser.add_argument("-output", help="Path to write transformed XML file")
    args = parser.parse_args()

    process_junit_xml(args.input, args.output)
    print(f"Transformed XML written to: {args.output}")

if __name__ == "__main__":
    main()