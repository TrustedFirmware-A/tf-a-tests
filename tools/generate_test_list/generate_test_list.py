#!/usr/bin/env python3
#
# Copyright (c) 2023 Google LLC. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Generates the same output as generate_test_list.pl, but using python.

Takes an xml file describing a list of testsuites as well as a skip list file
and outputs a src and header file that refers to those tests.
"""

# This script was linted and formatted using the following commands:
# autoflake -ir --remove-all-unused-imports --expand-star-imports \
#   --remove-duplicate-keys --remove-unused-variables tools/generate_test_list/
# isort tools/generate_test_list/
# black tools/generate_test_list/ --line-length 100
# flake8 tools/generate_test_list/ --max-line-length 100

import argparse
import os.path
import urllib.parse
import xml.etree.ElementInclude
import xml.parsers.expat
from dataclasses import dataclass
from typing import Dict, List
from xml.etree.ElementTree import Element, TreeBuilder

TESTS_LIST_H_TPL_FILENAME = "tests_list.h.tpl"
TESTCASE_COUNT_TEMPLATE = "{{testcase_count}}"

TESTS_LIST_C_TPL_FILENAME = "tests_list.c.tpl"
FUNCTION_PROTOTYPES_TEMPLATE = "{{function_prototypes}}"
TESTCASE_LISTS_TEMPLATE = "{{testcase_lists}}"
TESTSUITES_LIST_TEMPLATE = "{{testsuites_list}}"

XINCLUDE_INCLUDE = "xi:include"

MAX_EXPANSION_DEPTH = 5

# Intermediate repesentation classes.


@dataclass
class TestCase:
    """Class representing a single TFTF test case."""

    name: str
    function: str
    description: str = ""


@dataclass
class TestSuite:
    """Class representing a single TFTF test suite."""

    name: str
    description: str
    testcases: List[TestCase]


def find_element_with_name_or_return_none(iterable, name: str):
    """Looks through iterable for an element whose 'name' field matches name."""
    return next(filter(lambda x: x.name == name, iterable), None)


def parse_testsuites_element_into_ir(root: Element) -> List[TestSuite]:
    """Given the root of a parsed XML file, construct TestSuite objects."""
    testsuite_xml_elements = root.findall(".//testsuite")

    testsuites = []
    # Parse into IR
    for testsuite in testsuite_xml_elements:
        testcases = []
        for testcase in testsuite.findall("testcase"):
            testcases += [
                TestCase(
                    testcase.get("name"),
                    testcase.get("function"),
                    testcase.get("description", default=""),
                )
            ]
        testsuites += [TestSuite(testsuite.get("name"), testsuite.get("description"), testcases)]

    return testsuites


# In order to keep this script standalone (meaning no libraries outside of the
# standard library), we have to do our own assembling of the XML Elements. This
# is necessary because python doesn't give us a nice way to support external
# entity expansion. As such we have to use the low level expat parser and build
# the tree using TreeBuilder.


def parse_xml_no_xinclude_expansion(filename: str) -> Element:
    """Parse filename into an ElementTree.Element, following external entities."""
    xml_dir_root = os.path.dirname(filename)
    with open(filename) as fobj:
        xml_contents = fobj.read()

    parser = xml.parsers.expat.ParserCreate()
    parser.SetParamEntityParsing(xml.parsers.expat.XML_PARAM_ENTITY_PARSING_ALWAYS)

    global treebuilder
    treebuilder = TreeBuilder()
    global expansion_depth
    expansion_depth = 0

    def start_element_handler(name: str, attributes):
        # ElementInclude.include requires that the XInclude namespace is expanded.
        if name == "xi:include":
            name = "{http://www.w3.org/2001/XInclude}include"
        treebuilder.start(name, attributes)

    def end_element_handler(name: str):
        treebuilder.end(name)

    def external_entity_ref_handler(context, base, systemId, publicId):
        global expansion_depth

        external_entity_parser = parser.ExternalEntityParserCreate(context, "utf-8")
        assign_all_parser_callbacks(external_entity_parser)
        with open(os.path.join(xml_dir_root, systemId)) as fobj:
            sub_xml_contents = fobj.read()
            expansion_depth += 1
            if expansion_depth > MAX_EXPANSION_DEPTH:
                raise ValueError("Max entity expansion depth reached")

            external_entity_parser.Parse(sub_xml_contents, True)
            expansion_depth -= 1
        return 1

    def assign_all_parser_callbacks(p):
        p.StartElementHandler = start_element_handler
        p.EndElementHandler = end_element_handler
        p.ExternalEntityRefHandler = external_entity_ref_handler

    assign_all_parser_callbacks(parser)
    parser.Parse(xml_contents, True)

    return treebuilder.close()


# Older versions of python3 don't support ElementInclude.include's base_url
# kwarg. This callable class works around this.
# base_url allows XInclude paths relative to the toplevel XML file to be used.
class ElementIncludeLoaderAdapter:
    """Adapts between ElementInclude's loader interface and our XML parser."""

    def __init__(self, base_url: str):
        self.base_url = base_url

    def __call__(self, href: str, parse: str):
        if parse != "xml":
            raise ValueError("'parse' must be 'xml'")

        return parse_xml_no_xinclude_expansion(urllib.parse.urljoin(self.base_url, href))


def parse_testsuites_from_file(filename: str) -> List[TestSuite]:
    """Given an XML file, parse the contents into a List[TestSuite]."""
    root = parse_xml_no_xinclude_expansion(filename)

    base_url = os.path.abspath(filename)
    loader = ElementIncludeLoaderAdapter(base_url)
    xml.etree.ElementInclude.include(root, loader=loader)

    if root.tag == "testsuites":
        testsuites_xml_elements = [root]
    elif root.tag == "document":
        testsuites_xml_elements = root.findall("testsuites")
    else:
        raise ValueError(f"Unexpected root tag '{root.tag}' in {filename}")

    testsuites = []

    for testsuites_xml_element in testsuites_xml_elements:
        testsuites += parse_testsuites_element_into_ir(testsuites_xml_element)

    return testsuites


def check_validity_of_names(testsuites: List[TestSuite]):
    """Checks that all testsuite and testcase names are valid."""
    testsuite_name_set = set()
    for ts in testsuites:
        if "/" in ts.name:
            raise ValueError(f"ERROR: {args.xml_test_filename}: Invalid test suite name {ts.name}")

        if ts.name in testsuite_name_set:
            raise ValueError(
                f"ERROR: {args.xml_test_filename}: Can't have 2 test suites named " f"{ts.name}"
            )

        testsuite_name_set.add(ts.name)

        testcase_name_set = set()
        for tc in ts.testcases:
            if tc.name in testcase_name_set:
                raise ValueError(
                    f"ERROR: {args.xml_test_filename}: Can't have 2 tests named " f"{tc.name}"
                )

            testcase_name_set.add(tc.name)


def remove_skipped_tests(testsuites: List[TestSuite], skip_tests_filename: str):
    """Remove skipped tests from testsuites based on skip_tests_filename."""
    with open(skip_tests_filename) as skipped_file:
        skipped_file_lines = skipped_file.readlines()
        for i, l in enumerate(skipped_file_lines):
            line = l.strip()

            # Skip empty lines and comments
            if not line or line[0] == "#":
                continue

            testsuite_name, sep, testcase_name = line.partition("/")

            testsuite = find_element_with_name_or_return_none(testsuites, testsuite_name)

            if not testsuite:
                print(
                    f"WARNING: {skip_tests_filename}:{i + 1}: Test suite "
                    f"'{testsuite_name}' doesn't exist or has already been deleted."
                )
                continue

            if not testcase_name:
                print(f"INFO: Test suite '{testsuite_name}' will be skipped")
                testsuites = list(filter(lambda x: x.name != testsuite_name, testsuites))
                continue

            testcase = find_element_with_name_or_return_none(testsuite.testcases, testcase_name)
            if not testcase:
                print(
                    f"WARNING: {skip_tests_filename}:{i + 1}: Test case "
                    f"'{testsuite_name}/{testcase_name} doesn't exist or has already "
                    "been deleted"
                )
                continue

            print(f"INFO: Test case '{testsuite_name}/{testcase_name}' will be skipped.")
            testsuite.testcases.remove(testcase)

        return testsuites


def generate_function_prototypes(testcases: List[TestCase]):
    """Generates function prototypes for the provided list of testcases."""
    return [f"test_result_t {t.function}(void);" for t in testcases]


def generate_testcase_lists(testsuites: List[TestSuite]):
    """Generates the lists that enumerate the individual testcases in each testsuite."""
    testcase_lists_contents = []
    testcase_index = 0
    for i, testsuite in enumerate(testsuites):
        testcase_lists_contents += [f"\nconst test_case_t testcases_{i}[] = {{"]
        for testcase in testsuite.testcases:
            testcase_lists_contents += [
                f'  {{ {testcase_index}, "{testcase.name}", '
                f'"{testcase.description}", {testcase.function} }},'
            ]
            testcase_index += 1
        testcase_lists_contents += ["  { 0, NULL, NULL, NULL }"]
        testcase_lists_contents += ["};\n"]

    return testcase_lists_contents


def generate_testsuite_lists(testsuites: List[TestSuite]):
    """Generates the list of testsuites."""
    testsuites_list_contents = []
    testsuites_list_contents += ["const test_suite_t testsuites[] = {"]
    for i, testsuite in enumerate(testsuites):
        testsuites_list_contents += [
            f'  {{ "{testsuite.name}", "{testsuite.description}", testcases_{i} }},'
        ]
    testsuites_list_contents += ["  { NULL, NULL, NULL }"]
    testsuites_list_contents += ["};"]
    return testsuites_list_contents


def generate_file_from_template(
    template_filename: str, output_filename: str, template: Dict[str, str]
):
    """Given a template file, generate an output file based on template dictionary."""
    with open(template_filename) as template_fobj:
        template_contents = template_fobj.read()

    output_contents = template_contents
    for to_find, to_replace in template.items():
        output_contents = output_contents.replace(to_find, to_replace)

    with open(output_filename, "w") as output_fobj:
        output_fobj.write(output_contents)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "testlist_src_filename",
        type=str,
        help="Output source filename",
    )
    parser.add_argument(
        "testlist_hdr_filename",
        type=str,
        help="Output header filename",
    )
    parser.add_argument("xml_test_filename", type=str, help="Input xml filename")
    parser.add_argument(
        "--plat-skip-file",
        type=str,
        help="Filename containing tests to skip for this platform",
        dest="plat_skipped_list_filename",
        required=False,
    )
    parser.add_argument(
        "--arch-skip-file",
        type=str,
        help="Filename containing tests to skip for this architecture",
        dest="arch_skipped_list_filename",
        required=False,
    )
    args = parser.parse_args()

    testsuites = parse_testsuites_from_file(args.xml_test_filename)

    check_validity_of_names(testsuites)

    if args.plat_skipped_list_filename:
        testsuites = remove_skipped_tests(testsuites, args.plat_skipped_list_filename)

    if args.arch_skipped_list_filename:
        testsuites = remove_skipped_tests(testsuites, args.arch_skipped_list_filename)

    # Flatten all testcases
    combined_testcases = [tc for ts in testsuites for tc in ts.testcases]

    # Generate header file
    generate_file_from_template(
        os.path.join(os.path.dirname(__file__), TESTS_LIST_H_TPL_FILENAME),
        args.testlist_hdr_filename,
        {TESTCASE_COUNT_TEMPLATE: str(len(combined_testcases))},
    )

    # Generate the source file
    all_function_prototypes = generate_function_prototypes(combined_testcases)
    testcase_lists_contents = generate_testcase_lists(testsuites)
    testsuites_list_contents = generate_testsuite_lists(testsuites)

    generate_file_from_template(
        os.path.join(os.path.dirname(__file__), TESTS_LIST_C_TPL_FILENAME),
        args.testlist_src_filename,
        {
            FUNCTION_PROTOTYPES_TEMPLATE: "\n".join(all_function_prototypes),
            TESTCASE_LISTS_TEMPLATE: "\n".join(testcase_lists_contents),
            TESTSUITES_LIST_TEMPLATE: "\n".join(testsuites_list_contents),
        },
    )
