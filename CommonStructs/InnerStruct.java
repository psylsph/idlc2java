package CommonStructs;

import java.io.Serializable;

public record InnerStruct(int id, String name) implements Serializable {

    @Override
    public String toString() {
        return "InnerStruct[" +
            "id=" + id() +
            ", " + "name=" + name() +
            "]";
    }

}
