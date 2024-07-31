from .html_generator import HTMLGenerator
from .pdf_generator import PDFGenerator
from .mark_down_generator import MarkdownGenerator
from .exporter import *
from .types import *
from test_cases_generator.parser import Parser


class ExporterFactory():
    @staticmethod
    def get_exporter(type_: str) -> IExporter:
        if convert_str_to_exporter_type(type_) == ExporterType.PDF:
            return PDFGenerator()
        elif convert_str_to_exporter_type(type_) == ExporterType.HTML:
            return HTMLGenerator()
        elif convert_str_to_exporter_type(type_) == ExporterType.MARK_DOWN:
            return MarkdownGenerator()


def parse_yaml_to_documentation(parser: Parser):
    metadata = Metadata(
        parser.get_metadata()["description"],
        parser.get_metadata()["keywords"])

    arguments = {name: Argument(parser.get_types()[index], parser.get_sources()[index], parser.get_subset(
    )[index], parser.get_restrictions()[index]) for index, (name, arg_info) in enumerate(parser.get_arguments().items())}

    output = Output(parser.get_output()["type"], parser.get_output().get("subset"))

    # TODO: Implement this
    # examples = [
    #     Example(
    #         test['arguments'],
    #         test['should_pass'],
    #         test['description'],
    #         test.get('skipped', False),
    #         test.get('expected')) for test in parser.get_tests()]

    documentation = Documentation(
        name=parser.get_name(),
        helper_type=parser.get_helper_type(),
        is_variadic=parser.is_variadic(),
        metadata=metadata,
        arguments=arguments,
        output=output,
        general_restrictions=parser.get_general_restrictions_details(),
        examples=[]
    )

    return documentation
