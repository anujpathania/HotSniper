import java.io.*;
import java.lang.reflect.*;

public class ProbeMux {
  private static Probe[] probes;
  private static int iteration;
  private static String benchmark;

  public static void init() {
    String probesProperty = System.getProperty("probes");
    if (probesProperty == null) {
      System.err.println("WARNING: No probes configured, pass -Dprobes=Probe1,Probe2,...");
      probes = new Probe[0];
      return;
    }
    String[] probeNames = probesProperty.split(",");
    probes = new Probe[probeNames.length];
    int i = 0;
    for(String probeName: probeNames) {
      Class<? extends Probe> probeClass;
      String probeClassName = probeName + "Probe"; //"probe." + probeName + "Probe";
      try {
        probeClass = Class.forName(probeClassName).asSubclass(Probe.class);
      } catch (ClassNotFoundException cnfe) {
        throw new RuntimeException("Could not find probe class '" + probeClassName + "'", cnfe);
      }
      try {
        probes[i++] = probeClass.newInstance();
      } catch (Exception e) {
        throw new RuntimeException("Could not instantiate probe class '" + probeClassName + "'", e);
      }
    }
    for(i=0; i < probes.length; i++) {
      probes[i].init();
    }
  }

  public static void begin(String bm, boolean warmup) {
    benchmark = bm;
    iteration++;
    for(int i=0; i < probes.length; i++) {
      probes[i].begin(benchmark, iteration, warmup);
    }
  }
  
  public static void end(boolean warmup) {
    for(int i=probes.length-1; i>=0 ;i--) {
      probes[i].end(benchmark, iteration, warmup);
    }
    for(int i=probes.length-1; i>=0 ;i--) {
      probes[i].report(benchmark, iteration, warmup);
    }
  }

  public static void cleanup() {
    for(int i=0; i < probes.length; i++) {
      probes[i].cleanup();
    }
  }
}
