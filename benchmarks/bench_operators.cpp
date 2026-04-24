// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>
#include <parametric/core.hpp>
#include <parametric/operators.hpp>

class FiftyDoubles: public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&){

    }
    void TearDown(const ::benchmark::State&){}

    double d00 =  0; parametric::param<double> v00 = parametric::new_param( 0.);
    double d01 =  1; parametric::param<double> v01 = parametric::new_param( 1.);
    double d02 =  2; parametric::param<double> v02 = parametric::new_param( 2.);
    double d03 =  3; parametric::param<double> v03 = parametric::new_param( 3.);
    double d04 =  4; parametric::param<double> v04 = parametric::new_param( 4.);
    double d05 =  5; parametric::param<double> v05 = parametric::new_param( 5.);
    double d06 =  6; parametric::param<double> v06 = parametric::new_param( 6.);
    double d07 =  7; parametric::param<double> v07 = parametric::new_param( 7.);
    double d08 =  8; parametric::param<double> v08 = parametric::new_param( 8.);
    double d09 =  9; parametric::param<double> v09 = parametric::new_param( 9.);
    double d10 = 10; parametric::param<double> v10 = parametric::new_param(10.);
    double d11 = 11; parametric::param<double> v11 = parametric::new_param(11.);
    double d12 = 12; parametric::param<double> v12 = parametric::new_param(12.);
    double d13 = 13; parametric::param<double> v13 = parametric::new_param(13.);
    double d14 = 14; parametric::param<double> v14 = parametric::new_param(14.);
    double d15 = 15; parametric::param<double> v15 = parametric::new_param(15.);
    double d16 = 16; parametric::param<double> v16 = parametric::new_param(16.);
    double d17 = 17; parametric::param<double> v17 = parametric::new_param(17.);
    double d18 = 18; parametric::param<double> v18 = parametric::new_param(18.);
    double d19 = 19; parametric::param<double> v19 = parametric::new_param(19.);
    double d20 = 10; parametric::param<double> v20 = parametric::new_param(20.);
    double d21 = 11; parametric::param<double> v21 = parametric::new_param(21.);
    double d22 = 12; parametric::param<double> v22 = parametric::new_param(22.);
    double d23 = 13; parametric::param<double> v23 = parametric::new_param(23.);
    double d24 = 14; parametric::param<double> v24 = parametric::new_param(24.);
    double d25 = 15; parametric::param<double> v25 = parametric::new_param(25.);
    double d26 = 16; parametric::param<double> v26 = parametric::new_param(26.);
    double d27 = 17; parametric::param<double> v27 = parametric::new_param(27.);
    double d28 = 18; parametric::param<double> v28 = parametric::new_param(28.);
    double d29 = 19; parametric::param<double> v29 = parametric::new_param(29.);
    double d30 = 10; parametric::param<double> v30 = parametric::new_param(30.);
    double d31 = 11; parametric::param<double> v31 = parametric::new_param(31.);
    double d32 = 12; parametric::param<double> v32 = parametric::new_param(32.);
    double d33 = 13; parametric::param<double> v33 = parametric::new_param(33.);
    double d34 = 14; parametric::param<double> v34 = parametric::new_param(34.);
    double d35 = 15; parametric::param<double> v35 = parametric::new_param(35.);
    double d36 = 16; parametric::param<double> v36 = parametric::new_param(36.);
    double d37 = 17; parametric::param<double> v37 = parametric::new_param(37.);
    double d38 = 18; parametric::param<double> v38 = parametric::new_param(38.);
    double d39 = 19; parametric::param<double> v39 = parametric::new_param(39.);
    double d40 = 10; parametric::param<double> v40 = parametric::new_param(40.);
    double d41 = 11; parametric::param<double> v41 = parametric::new_param(41.);
    double d42 = 12; parametric::param<double> v42 = parametric::new_param(42.);
    double d43 = 13; parametric::param<double> v43 = parametric::new_param(43.);
    double d44 = 14; parametric::param<double> v44 = parametric::new_param(44.);
    double d45 = 15; parametric::param<double> v45 = parametric::new_param(45.);
    double d46 = 16; parametric::param<double> v46 = parametric::new_param(46.);
    double d47 = 17; parametric::param<double> v47 = parametric::new_param(47.);
    double d48 = 18; parametric::param<double> v48 = parametric::new_param(48.);
    double d49 = 19; parametric::param<double> v49 = parametric::new_param(49.);


};

BENCHMARK_DEFINE_F(FiftyDoubles, baseline_addition)(benchmark::State& state) {
    for (auto _ : state) {
        double res = d00 + d01 + d02 + d03 + d04 + d05 + d06 + d07 + d08 + d09
                   + d10 + d11 + d12 + d13 + d14 + d15 + d16 + d17 + d18 + d19
                   + d20 + d21 + d22 + d23 + d24 + d25 + d26 + d27 + d28 + d29
                   + d30 + d31 + d32 + d33 + d34 + d35 + d36 + d37 + d38 + d39
                   + d40 + d41 + d42 + d43 + d44 + d45 + d46 + d47 + d48 + d49;
        benchmark::DoNotOptimize(res);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, baseline_addition);

BENCHMARK_DEFINE_F(FiftyDoubles, baseline_multiplication)(benchmark::State& state) {
    for (auto _ : state) {
        double res = d00 * d01 * d02 * d03 * d04 * d05 * d06 * d07 * d08 * d09
                   * d10 * d11 * d12 * d13 * d14 * d15 * d16 * d17 * d18 * d19
                   * d20 * d21 * d22 * d23 * d24 * d25 * d26 * d27 * d28 * d29
                   * d30 * d31 * d32 * d33 * d34 * d35 * d36 * d37 * d38 * d39
                   * d40 * d41 * d42 * d43 * d44 * d45 * d46 * d47 * d48 * d49;
        benchmark::DoNotOptimize(res);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, baseline_multiplication);

BENCHMARK_DEFINE_F(FiftyDoubles, param_create_addition)(benchmark::State& state) {
    for (auto _ : state) {
        auto xpr = v00 + v01 + v02 + v03 + v04 + v05 + v06 + v07 + v08 + v09
                 + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19
                 + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29
                 + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39
                 + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
        benchmark::DoNotOptimize(xpr);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_create_addition);


BENCHMARK_DEFINE_F(FiftyDoubles, param_create_multiplication)(benchmark::State& state) {
    for (auto _ : state) {
        auto xpr = v00 * v01 * v02 * v03 * v04 * v05 * v06 * v07 * v08 * v09
                 * v10 * v11 * v12 * v13 * v14 * v15 * v16 * v17 * v18 * v19
                 * v20 * v21 * v22 * v23 * v24 * v25 * v26 * v27 * v28 * v29
                 * v30 * v31 * v32 * v33 * v34 * v35 * v36 * v37 * v38 * v39
                 * v40 * v41 * v42 * v43 * v44 * v45 * v46 * v47 * v48 * v49;
        benchmark::DoNotOptimize(xpr);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_create_multiplication);

BENCHMARK_DEFINE_F(FiftyDoubles, param_eval_addition)(benchmark::State& state) {
    auto xpr = v00 + v01 + v02 + v03 + v04 + v05 + v06 + v07 + v08 + v09
             + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19
             + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29
             + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39
             + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
    for (auto _ : state) {
        xpr.node_pointer()->invalidate();
        double res = xpr;
        benchmark::DoNotOptimize(res);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_eval_addition);

BENCHMARK_DEFINE_F(FiftyDoubles, param_eval_multiplication)(benchmark::State& state) {
    auto xpr = v00 * v01 * v02 * v03 * v04 * v05 * v06 * v07 * v08 * v09
             * v10 * v11 * v12 * v13 * v14 * v15 * v16 * v17 * v18 * v19
             * v20 * v21 * v22 * v23 * v24 * v25 * v26 * v27 * v28 * v29
             * v30 * v31 * v32 * v33 * v34 * v35 * v36 * v37 * v38 * v39
             * v40 * v41 * v42 * v43 * v44 * v45 * v46 * v47 * v48 * v49;

    for (auto _ : state) {
        xpr.node_pointer()->invalidate();
        double res = xpr;
        // this benchmark is to micro to pause and resume timing for invalidation:
        // https://github.com/google/benchmark/issues/797
        benchmark::DoNotOptimize(res);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_eval_multiplication);

BENCHMARK_DEFINE_F(FiftyDoubles, param_invalidate_addition)(benchmark::State& state) {
    auto xpr = v00 * v01 * v02 * v03 * v04 * v05 * v06 * v07 * v08 * v09
             * v10 * v11 * v12 * v13 * v14 * v15 * v16 * v17 * v18 * v19
             * v20 * v21 * v22 * v23 * v24 * v25 * v26 * v27 * v28 * v29
             * v30 * v31 * v32 * v33 * v34 * v35 * v36 * v37 * v38 * v39
             * v40 * v41 * v42 * v43 * v44 * v45 * v46 * v47 * v48 * v49;
    for (auto _ : state) {
        xpr.node_pointer()->invalidate();
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_invalidate_addition);

BENCHMARK_DEFINE_F(FiftyDoubles, param_invalidate_multiplication)(benchmark::State& state) {
    auto xpr = v00 * v01 * v02 * v03 * v04 * v05 * v06 * v07 * v08 * v09
             * v10 * v11 * v12 * v13 * v14 * v15 * v16 * v17 * v18 * v19
             * v20 * v21 * v22 * v23 * v24 * v25 * v26 * v27 * v28 * v29
             * v30 * v31 * v32 * v33 * v34 * v35 * v36 * v37 * v38 * v39
             * v40 * v41 * v42 * v43 * v44 * v45 * v46 * v47 * v48 * v49;

    for (auto _ : state) {
        xpr.node_pointer()->invalidate();
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_invalidate_multiplication);

BENCHMARK_DEFINE_F(FiftyDoubles, param_retrieve_addition)(benchmark::State& state) {
    auto xpr = v00 + v01 + v02 + v03 + v04 + v05 + v06 + v07 + v08 + v09
             + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19
             + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29
             + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39
             + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
    double res1 = xpr;
    benchmark::DoNotOptimize(res1);
    for (auto _ : state) {
        double res2 = xpr;
        benchmark::DoNotOptimize(res2);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_retrieve_addition);


BENCHMARK_DEFINE_F(FiftyDoubles, param_retrieve_multiplication)(benchmark::State& state) {
    auto xpr = v00 * v01 * v02 * v03 * v04 * v05 * v06 * v07 * v08 * v09
             * v10 * v11 * v12 * v13 * v14 * v15 * v16 * v17 * v18 * v19
             * v20 * v21 * v22 * v23 * v24 * v25 * v26 * v27 * v28 * v29
             * v30 * v31 * v32 * v33 * v34 * v35 * v36 * v37 * v38 * v39
             * v40 * v41 * v42 * v43 * v44 * v45 * v46 * v47 * v48 * v49;
    double res1 = xpr;
    benchmark::DoNotOptimize(res1);
    for (auto _ : state) {
        double res2 = xpr;
        benchmark::DoNotOptimize(res2);
    };
}
BENCHMARK_REGISTER_F(FiftyDoubles, param_retrieve_multiplication);
