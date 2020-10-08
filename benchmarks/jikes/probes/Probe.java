 
public interface Probe {
  public void init();
  public void begin(String benchmark, int iteration, boolean warmup);
  public void end(String benchmark, int iteration, boolean warmup);
  public void report(String benchmark, int iteration, boolean warmup);
  public void cleanup();
}
