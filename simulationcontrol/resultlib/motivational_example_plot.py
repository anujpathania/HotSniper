import os.path, io, sys, gzip
import re, cv2
import math

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

from datetime import datetime

from matplotlib.axes import Axes

from pprint import pprint


RESULTS_DIR = os.path.join(os.getenv('GRAPHITE_ROOT'), 'results')

class ExpData:
    def __init__(self):
        self.output_file: str = ''
        self.log_file: str = ''
        self.heat_file: str =  ''
        self.hb_df = pd.DataFrame()


def compile_testdata(label: str) -> ExpData:
    data_path =  ''
    
    for dirname in sorted(os.listdir(RESULTS_DIR)):
        if label in dirname:
            data_path= os.path.join(RESULTS_DIR, dirname)

    if data_path == '': 
        return None
    
    # print(data_path)

    data: ExpData = ExpData()           
        
    # save reference to output file and log_file
    for file in os.listdir(data_path):
        # print(file)

        if 'output.txt' in file:
            data.output_file = os.path.join(data_path, file)

        if 'execution.log.gz' in file:
            data.log_file = os.path.join(data_path, file)

        if 'PeriodicThermal.log.gz' in file:
            data.heat_file = os.path.join(data_path, file)
            
        if 'hb.log' in file:
            file_df = pd.read_csv(os.path.join(data_path, file), sep='\t')

            data.hb_df = pd.concat([data.hb_df, file_df])

    return data


def parse_images(video_path: str) -> list:
    encoded_vid = cv2.VideoCapture(video_path)
            
    frames = []
    succ, img = encoded_vid.read()
    while succ:
        frames.append(img) 
        succ, img = encoded_vid.read()

    return frames


def perforation_qos_loss_plot(benchmark: str, data: ExpData, ax: Axes):
    perforation_noise = []

    for _ in data.idx:
        perforation_noise.append(0.0)
    
    output_comparison = []
        
    if benchmark in {'blackscholes', 'bodytrack'}: # text files with the outputs on lines
        for file in data.output_files:
            run_out = []
            f = open(file)
            for line in f.readlines():
                run_out += [float(num) for num in  re.findall(r'-?\b\d+\.?\d*\b', line)]
            
            output_comparison.append(run_out)
    
    elif benchmark in {'canneal'}: # one number in the execution log.
        for file in data.output_files: 
            execution_log =  io.TextIOWrapper(gzip.open(file, 'r'), encoding="utf-8")
            
            for line in execution_log:
                m = re.search(r'Final routing is: (\d+\.\d+)', line)
                if m is not None:
                    output_comparison.append([float(m.group(1))])
                    break
    
    elif benchmark in {'swaptions'}: # one number in the execution log.
        for file in data.output_files: 
            execution_log =  io.TextIOWrapper(gzip.open(file, 'r'), encoding="utf-8")
            
            run_out = []
            for line in execution_log:
                m = re.search(r'SwaptionPrice: (\d+\.\d+) StdError: (\d+\.\d+)', line)
                if m is not None:
                    run_out.append(float(m.group(1)))
                    run_out.append(float(m.group(2)))
                
            output_comparison.append(run_out)

    elif benchmark in {'x264'}: # decompose video into frames and then calculate the noise.
        ref_file = data.output_files[0]
        ref_images = parse_images(ref_file) # maybe this should be the original video?
        
        for run_file in data.output_files: 
            run_out = []
            run_images = parse_images(run_file)
            
            run_out.append(os.path.getsize(run_file))

            for ref, run in zip(ref_images, run_images):
                run_out.append(psnr(ref, run))

            print(run_out)
            output_comparison.append(run_out)

    else:
        ax.set_title(name + " QoS on simsmall")
        ax.set_ylabel("% noise")
        ax.set_xlabel("perforation rate")
        return
    
    reference = output_comparison[0]
    for index, output in enumerate(output_comparison):
        perforation_noise[index] = 100 * (abs(sum(reference)- sum(output)) / abs(sum(reference)))


def get_output(data : ExpData):
     
    execution_log =  io.TextIOWrapper(gzip.open(data.log_file, 'r'), encoding="utf-8")
    
    run_out = []
    for line in execution_log:
        m = re.search(r'SwaptionPrice: (\d+\.\d+) StdError: (\d+\.\d+)', line)
        if m is not None:
            run_out.append(float(m.group(1)))
            # run_out.append(float(m.group(2)))
                
    
    return run_out


def get_peak_temperature_traces(data: ExpData):
    heat_log =  io.TextIOWrapper(gzip.open(data.heat_file, 'r'), encoding="utf-8")
    traces = []

    for line in heat_log:
        try:
            vs = [1 * float(v) for v in line.split()]
            traces.append(vs)
        except:
            continue

    traces = list(zip(*traces))

    peak = []
    for values in zip(*traces):
        peak.append(max(values))
    return peak

def heat_speedup_accuracy_plot(data: ExpData, ax: Axes, baseline_output):

    qos = abs(sum(get_output(data)) - sum(baseline_output) / sum(baseline_output))

    print(qos)
    peaks = get_peak_temperature_traces(data)

    sns.lineplot(y=peaks, x=[i for i, v in enumerate(peaks)], ax=ax)
    return

    # make point estimate of the output error compared to the base line.
    # graph the max temperature.



three_ghz = compile_testdata("results_2024-05-13_13.25_3.0GHz+maxFreq_parsec-swaptions-simsmall-3_exp_temp_pr:0")
two_ghz = compile_testdata("exp_qos_pr:0,0")
two_perf_1 = compile_testdata("exp_qos_pr:30,30")
two_perf_2 = compile_testdata("exp_qos_pr:32,32")
two_asym_1 = compile_testdata("exp_qos_pr:35,10")
two_asym_2 = compile_testdata("exp_qos_pr:40,5")

figure_data = [three_ghz, two_ghz, two_perf_1, two_perf_2, two_asym_1, two_asym_2]

baseline_output = get_output(two_ghz)

fig, axs = plt.subplots(2, 3, figsize=(15, 10))
flat_ax = (axs.flatten())

for i, ax in enumerate(flat_ax):
    print(figure_data[i].log_file)
    heat_speedup_accuracy_plot(figure_data[i], ax, baseline_output)
    
plt.tight_layout()
plt.savefig("motivational_example_{}.pdf".format(datetime.today()))
