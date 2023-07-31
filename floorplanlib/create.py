#!/usr/bin/env python3

import abc
import argparse
import os
import re
import subprocess
import sys

# Floorplan creator script taken from the CoMet project.
# Removed memory, connection and TIM layers 

SILICON_SPECIFIC_HEAT_CAPACITY = 1.75e6
SILICON_THERMAL_RESISTIVITY = 0.01

TIM_SPECIFIC_HEAT_CAPACITY = 4e6
TIM_THERMAL_RESISTIVITY = 0.25

AIR_SPECIFIC_HEAT_CAPACITY = 2875000
AIR_THERMAL_RESISTIVITY = 0.13


HERE = os.path.dirname(os.path.abspath(__file__))


class Length(object):
    """
    distance stored in exact numbers in micrometers
    """
    def __init__(self, micrometers):
        self.micrometers = int(micrometers + 0.5)

    def __eq__(self, other):
        return self.micrometers == other.micrometers
    def __gt__(self, other):
        return self.micrometers > other.micrometers
    def __ge__(self, other):
        return self.micrometers >= other.micrometers

    def __mul__(self, v):
        return Length(v * self.micrometers)
    __rmul__ = __mul__

    def __add__(self, v):
        return Length(self.micrometers + v.micrometers)
    def __sub__(self, v):
        return Length(self.micrometers - v.micrometers)

    @property
    def meters(self):
        return self.micrometers / 1e6

    def __str__(self):
        return f'{self.meters:.6f}m'

    def __repr__(self):
        return f'Length({self.meters:.6f}m)'

    @classmethod
    def from_meters(self, v):
        return Length(v * 1e6)


def length(s):
    """ Parse length units. """
    units = {
        'm': 1e6,
        'dm': 1e5,
        'cm': 1e4,
        'mm': 1e3,
        'um': 1,
    }
    m = re.match(r'(?P<nb>\d+(\.\d+)?)(?P<unit>[a-z]+)', s)
    if not m:
        raise argparse.ArgumentTypeError(f'{s} is not a valid length. Valid examples are: 0.001m, 1mm, 980um')
    nb = float(m['nb'])
    if nb < 0:
        raise argparse.ArgumentTypeError('length cannot be negative')
    if m['unit'] not in units:
        units_str = ', '.join(units.keys())
        raise argparse.ArgumentTypeError(f'{m["unit"]} is not a valid unit of length. Valid units are: {units_str}')
    return Length(round(nb * units[m['unit']]))


def dimension_2d(s):
    pat = re.compile(r"(\d+)x(\d+)")
    m = pat.match(s)
    if not m:
        raise argparse.ArgumentTypeError('invalid format. Valid examples: 3x4, 8x8')
    return (int(m.group(1)), int(m.group(2)))


def dimension_3d(s):
    pat = re.compile(r"(\d+)x(\d+)x(\d+)")
    m = pat.match(s)
    if not m:
        raise argparse.ArgumentTypeError('invalid format. Valid examples: 4x4x2, 8x8x1')
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def dimension_2d_or_3d(s):
    try:
        return dimension_3d(s)
    except argparse.ArgumentTypeError:
        try:
            return dimension_2d(s)
        except argparse.ArgumentTypeError:
            raise argparse.ArgumentTypeError('invalid format. Valid examples: 4x4, 8x8x1')


def dimension_extend_to_3d(s):
    try:
        return dimension_3d(s)
    except argparse.ArgumentTypeError:
        try:
            a, b = dimension_2d(s)
            return a, b, 1
        except argparse.ArgumentTypeError:
            raise argparse.ArgumentTypeError('invalid format. Valid examples: 4x4, 8x8x1')


class FloorplanComponent(object):
    def __init__(self, name, width, height, left, bottom):
        self.name = name
        self.width = width
        self.height = height
        self.left = left
        self.bottom = bottom

    def format(self, endline=False):
        s = '{}\t{:.6f}\t{:.6f}\t{:.6f}\t{:.6f}'.format(
            self.name, self.width.meters, self.height.meters, self.left.meters, self.bottom.meters)
        if endline:
            s += '\n'
        return s


class Floorplan(object):
    def __init__(self, components):
        self.components = components

    @property
    def width(self):
        return max(c.left + c.width for c in self.components) - self.left

    @property
    def height(self):
        return max(c.bottom + c.height for c in self.components) - self.bottom

    @property
    def left(self):
        return min(c.left for c in self.components)

    @property
    def bottom(self):
        return min(c.bottom for c in self.components)


def floorplan_file(s):
    components = []
    if not os.path.exists(s):
        raise argparse.ArgumentTypeError('file does not exist')
    with open(s) as f:
        for nb, line in enumerate(f):
            if line.strip().startswith('#'):
                continue
            else:
                pat = re.compile(r"([A-Za-z\-_0-9]+)\t(\d+\.\d+)\t(\d+\.\d+)\t(\d+\.\d+)\t(\d+\.\d+)")
                m = pat.match(line)
                if not m:
                    raise argparse.ArgumentTypeError(f'invalid floorplan file, parsing error on line {nb+1}')
                name = m.group(1)
                width = Length.from_meters(float(m.group(2)))
                height = Length.from_meters(float(m.group(3)))
                left = Length.from_meters(float(m.group(4)))
                bottom = Length.from_meters(float(m.group(5)))
                components.append(FloorplanComponent(name, width, height, left, bottom))
    return Floorplan(components)


class ThermalLayer(abc.ABC):
    """ base class for all thermal layers """
    def __init__(self, name):
        self.name = name

    def _get_floorplan_filename(self, directory):
        return os.path.join(directory, f'{self.name}.flp')

    def get_layer_configuration_string(self, directory, nb):
        return f'''\
# Layer "{self.name}"
{nb}
Y
{"Y" if self._has_power_consumption() else "N"}
{self._specific_heat_capacity()}
{self._thermal_resistivity()}
{self._thickness().meters}
{os.path.abspath(self._get_floorplan_filename(directory))}

'''

    @abc.abstractproperty
    def total_width(self):
        return self.elements[0] * self.element_width

    @abc.abstractproperty
    def total_height(self):
        return self.elements[1] * self.element_height

    @abc.abstractmethod
    def write_floorplan(self, directory):
        pass

    @abc.abstractmethod
    def _has_power_consumption(self):
        pass

    @abc.abstractmethod
    def _specific_heat_capacity(self):
        pass

    @abc.abstractmethod
    def _thermal_resistivity(self):
        pass

    @abc.abstractmethod
    def _thickness(self):
        pass


class SimpleLayer(ThermalLayer):
    """ base class for simple layers containing only one rectangular grid """
    def __init__(self, elements, element_width, element_height, thickness, name, nb_offset=0, pos_offset=None, subcomponent_template=None):
        super().__init__(name=name)
        self.elements = elements
        self.element_width = element_width
        self.element_height = element_height
        self.thickness = thickness
        self.nb_offset = nb_offset
        self.pos_offset = pos_offset if pos_offset is not None else (Length(0), Length(0))
        if subcomponent_template is not None:
            assert (subcomponent_template.left, subcomponent_template.bottom) == (Length(0), Length(0))
            assert (subcomponent_template.width, subcomponent_template.height) == (self.element_width, self.element_height)
        self.subcomponent_template = subcomponent_template

    @property
    def total_width(self):
        return self.elements[0] * self.element_width

    @property
    def total_height(self):
        return self.elements[1] * self.element_height

    def _thickness(self):
        return self.thickness

    def create_floorplan_elements(self):
        elements = []
        for y in range(self.elements[1]):
            for x in range(self.elements[0]):
                element_nb = self.nb_offset + y * self.elements[0] + x
                element_id = f'{self._get_element_identifier()}_{element_nb}'
                left = x * self.element_width + self.pos_offset[0]
                bottom = y * self.element_height + self.pos_offset[1]
                if self.subcomponent_template is None:
                    elements.append(
                        FloorplanComponent(
                            element_id,
                            self.element_width,
                            self.element_height,
                            left,
                            bottom))
                else:
                    for component in self.subcomponent_template.components:
                        subcomponent_id = f'{element_id}_{component.name}'
                        elements.append(
                            FloorplanComponent(
                                subcomponent_id,
                                component.width,
                                component.height,
                                left + component.left,
                                bottom + component.bottom))
        return ''.join(e.format(endline=True) for e in elements)

    def write_floorplan(self, directory):
        with open(self._get_floorplan_filename(directory), 'w') as f:
            f.write('# Line Format: <unit-name>\\t<width>\\t<height>\\t<left-x>\\t<bottom-y>\n')
            f.write(self.create_floorplan_elements())

    @abc.abstractmethod
    def _get_element_identifier(self):
        pass


class CoreLayer(SimpleLayer):
    """ a rectangular layer of cores """
    def _get_element_identifier(self):
        return 'C'

    def _has_power_consumption(self):
        return True

    def _specific_heat_capacity(self):
        return SILICON_SPECIFIC_HEAT_CAPACITY

    def _thermal_resistivity(self):
        return SILICON_THERMAL_RESISTIVITY


class ThermalStack(object):
    def __init__(self, name, has_heatsink=True):
        self.name = name
        self.has_heatsink = has_heatsink
        self.layers = []

    def add_layer(self, layer):
        self.layers.append(layer)

    def _write_hotspot_config(self, directory):
        with open(os.path.join(HERE, 'hotspot.config.tmpl'), 'r') as f:
            raw_content = f.read()
        chip_size = max(self.layers[0].total_width, self.layers[0].total_height).meters
        formatted_content = raw_content.format(
            s_solder=chip_size + 0.001,
            s_sub=chip_size + 0.02,
            s_spreader=chip_size + 0.02,
            s_sink=chip_size + 0.04,
            t_sink=0.0069 if self.has_heatsink else 0.00001,
        )
        with open(os.path.join(directory, f'{self.name}.hotspot_config'), 'w') as f:
            f.write(formatted_content)

    def _write_configuration_help(self, directory):
        pass  # TODO: implement

    def write_files(self, directory):
        for l in self.layers[1:]:
            assert l.total_width == self.layers[0].total_width
            assert l.total_height == self.layers[1].total_height

        if not os.path.exists(directory):
            os.makedirs(directory)

        for layer in self.layers:
            layer.write_floorplan(directory)
            # flp_to_pdf(layer._get_floorplan_filename(directory))  does not work due to fig2ps errors
        self._write_hotspot_config(directory)
        self._write_configuration_help(directory)


def flp_to_pdf(filename):
    HOTSPOT_PATH = os.path.join(HERE, '..', 'hotspot_tool')

    assert filename[-4:] == '.flp'
    pdf_filename = filename[:-4] + '.pdf'

    tofig = subprocess.Popen([os.path.join(HOTSPOT_PATH, 'tofig.pl'), filename], stdout=subprocess.PIPE)
    fig2dev = subprocess.Popen(['fig2dev', '-L', 'ps'], stdin=tofig.stdout, stdout=subprocess.PIPE)  # crashes
    ps2pdf = subprocess.Popen(['ps2pdf', '-', pdf_filename], stdin=fig2dev.stdout)
    tofig.stdout.close()
    fig2dev.stdout.close()


def main():
    parser = argparse.ArgumentParser()
    required = parser.add_argument_group('required named arguments')
    cores = parser.add_argument_group('cores')
    cores.add_argument("--cores", help="number of cores", type=dimension_extend_to_3d, required=True)
    cores.add_argument("--corex", help="size of each core (in dimension x)", type=length)
    cores.add_argument("--corey", help="size of each core (in dimension y)", type=length)
    cores.add_argument("--core_thickness", help="thickness of the core silicon layer", type=length, required=False, default='50um')
    cores.add_argument("--subcore-template", help="template for sub-core components", type=floorplan_file, required=False)
    required.add_argument("--out", help="directory in which the floorplan is stored", required=True)
    args = parser.parse_args()    

    cores_per_layer = args.cores[0] * args.cores[1]
    cores_2d = (args.cores[0], args.cores[1])

    if args.subcore_template is not None:
        if args.subcore_template.left != Length(0) or args.subcore_template.bottom != Length(0):
            parser.error('subcore-template must be positioned bottom left')

        print("Setting corex to {} and corey to {} using subcore-template width and height".format(
            args.subcore_template.width,
            args.subcore_template.height))
        args.corex = args.subcore_template.width
        args.corey = args.subcore_template.height

    flp_name = os.path.basename(args.out)
    core = ThermalStack(flp_name)
    for i in range(args.cores[2]):
        core.add_layer(CoreLayer(cores_2d, args.corex, args.corey, args.core_thickness, name=flp_name, nb_offset=i*cores_per_layer, subcomponent_template=args.subcore_template))
    core.write_files(args.out)

    with open(os.path.join(args.out, 'commandline.txt'), 'w') as f:
        f.write(f'''\
# command used to create these files:
{" ".join(sys.argv)}
''')


if __name__ == '__main__':
    main()
