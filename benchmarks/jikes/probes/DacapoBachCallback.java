
import org.dacapo.harness.Callback;
import org.dacapo.harness.CommandLineArgs;

public class DacapoBachCallback extends Callback {
  public DacapoBachCallback(CommandLineArgs cla) {
    super(cla);
    ProbeMux.init();
  }

  public void start(String benchmark) {
    ProbeMux.begin(benchmark, isWarmup());
    super.start(benchmark);
  }

  /* Immediately after the end of the benchmark */
  public void stop() {
    super.stop();
    ProbeMux.end(isWarmup());
    if (!isWarmup()) {
      ProbeMux.cleanup();
    }
  }
}
