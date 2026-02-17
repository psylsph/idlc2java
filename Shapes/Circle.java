package Shapes;

import java.io.Serializable;

public record Circle(int id, Point center, double radius, String color, java.util.List<Double> points) implements Serializable {

    @Override
    public String toString() {
        return "Circle[" +
            "id=" + id() +
            ", " + "center=" + center() +
            ", " + "radius=" + radius() +
            ", " + "color=" + color() +
            ", " + "points=" + points() +
            "]";
    }

}
