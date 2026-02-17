package Shapes;

import java.io.Serializable;

public record Point(int x, int y) implements Serializable {

    @Override
    public String toString() {
        return "Point[" +
            "x=" + x() +
            ", " + "y=" + y() +
            "]";
    }

}
