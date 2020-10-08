import harness.Callback;

public class PJBB2005Callback extends Callback {

  public PJBB2005Callback() {
    super();
    ProbeMux.init();
  }

  public void begin(int iteration, boolean warmup) {
    ProbeMux.begin("pjbb2005", warmup);
    super.begin(iteration, warmup);
  }

  public void end(int iteration, boolean warmup) {
    super.end(iteration, warmup);
    ProbeMux.end(warmup);
    if (!warmup) ProbeMux.cleanup();
  }
}
