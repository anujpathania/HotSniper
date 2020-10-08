SOURCES := $(filter-out PJBB2005Callback.java,${shell find *.java})
OBJECTS := $(patsubst %.java,%.class,$(SOURCES))
TARGET := probes.jar

all: $(TARGET)

$(TARGET): $(OBJECTS)
	jar cf $@ $(OBJECTS)

%.class: %.java
	javac -classpath .:../data/dacapo-9.12-bach.jar:../data/dacapo-2006-10-MR2.jar $<

clean:
	rm $(TARGET) $(OBJECTS)
