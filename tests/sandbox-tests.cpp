#include <catch.hpp>

#include <elegant/hdf5/hdf5>
#include <armadillo>

using namespace std;
using namespace elegant::hdf5;
using namespace arma;

SCENARIO("Sandbox tests", "[sandbox]") {
    GIVEN("a truncated file") {
        File file("armadillos.h5");
        mat A {{1,2,3},{4,5,6}};
        file["lol"] = A;
        mat B = file["lol"];
        cout << A << endl;
        cout << B << endl;

        cube C = ones(3,2,4);
        cout << C << endl;
        file["woop"] = C;

        cube D = file["woos"];
        cout << D << endl;

    }
//    GIVEN("a read-write file") {
//        File file("/home/svenni/tmp/new.h5", File::OpenMode::ReadWrite);
////        WHEN("writing an operation result") {
////            mat A = ones(2, 4);
////            mat B = ones(2, 4);
////            file["my_add"] = A + 2*B;
////            THEN("the result should be read back") {
////                mat C = A + 2*B;
////                mat D = file["my_add"];
////                REQUIRE(0 == Approx(max(max(abs(C - D)))));
////            }
////        }
//        WHEN("writing other stuff") {
//            cube cu(4000, 200, 6);
//            mat ma(4000, 200);
//            for(unsigned int i = 0; i < cu.n_slices; i++) {
//                for(unsigned int j = 0; j < cu.n_rows; j++) {
//                    for(unsigned int k = 0; k < cu.n_cols; k++) {
//                        double value = 0.00000001 * (k + j * cu.n_cols + i * cu.n_cols * cu.n_rows);
//                        cu(j, k, i) = value;
//                        ma(j, k) = value;
//                    }
//                }
//            }
//            file["cubic"] = cu;
//            file["cubic"].attribute("milad") = 92.5;
//            file["cubic"].attribute("some_number") = 410.87;
//            file["cubic"].attribute("blabla") = 912.5;
//            file["matic"] = ma;
////            cout << cu << endl;
//            THEN("the same should be read back") {
//                cube cur = file["cubic"];
//                cube diff = cu - cur;
//                diff = abs(diff);
//                double diffs = diff.max();
//                REQUIRE(0 == Approx(diffs));
//            }
//        }
//    }
}
