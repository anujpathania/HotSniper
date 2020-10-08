import java.lang.reflect.*;

public class ReplayProbe implements Probe {
  private Method compileAll;
  private static final String PRECOMPILE_CLASS = "org.jikesrvm.adaptive.recompilation.BulkCompile";
  private static final String PRECOMPILE_METHOD = "compileAllMethods";

  public void init() {
    try {
      Class preCompile = Class.forName(PRECOMPILE_CLASS);
      compileAll = preCompile.getMethod(PRECOMPILE_METHOD);
    } catch (Exception e) {
	throw new RuntimeException("Unable to find "+PRECOMPILE_CLASS+"."+PRECOMPILE_METHOD+"()", e);
    }
  }
  public void cleanup() {}

  public void begin(String benchmark, int iteration, boolean warmup) {
  }

  public void end(String benchmark, int iteration, boolean warmup) {
    if (iteration != 1) return;
    try {
      System.out.println("Replay probe end(), iteration "+iteration+" about to recompile...");
      compileAll.invoke(null);
    } catch (Exception e) {
      throw new RuntimeException("Error running ReplayProbe.end()", e);
    }
  }

  public void report(String benchmark, int iteration, boolean warmup) {}
}