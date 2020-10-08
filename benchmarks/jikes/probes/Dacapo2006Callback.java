import dacapo.Callback;

public class Dacapo2006Callback extends Callback {
  public Dacapo2006Callback() {
    super();
    ProbeMux.init();
  }

  public void startWarmup(String benchmark) {
    ProbeMux.begin(benchmark, true);
    super.startWarmup(benchmark);
  }

  public void stopWarmup() {
    super.stopWarmup();
    ProbeMux.end(true);
  }
  public void start(String benchmark) {
    ProbeMux.begin(benchmark, false);
    super.start(benchmark);
  }

  public void stop() {
    super.stop();
    ProbeMux.end(false);
    ProbeMux.cleanup();
  }
}
