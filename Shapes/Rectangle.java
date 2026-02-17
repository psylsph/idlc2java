package Shapes;

import java.io.Serializable;

public record Rectangle(int id, Point top_left, Point bottom_right, String label) implements Serializable {

    @Override
    public String toString() {
        return "Rectangle[" +
            "id=" + id() +
            ", " + "top_left=" + top_left() +
            ", " + "bottom_right=" + bottom_right() +
            ", " + "label=" + label() +
            "]";
    }

}
