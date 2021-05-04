from conans import ConanFile, CMake

class AshConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "boost/1.73.0",
        "spdlog/1.8.1",
        "simple-websocket-server/2.0.2",
        "asio/1.18.0",
        "nlohmann_json/3.7.0",
        "cryptopp/8.2.0",
        "range-v3/0.10.0",
        "leveldb/1.22"
    )

    generators = "cmake"

    default_options = {
        "boost:shared": False,
        "boost:without_test": False,
        "boost:without_thread": False,
        "boost:without_filesystem": False,
        "boost:without_system": False,
        "boost:without_stacktrace": False,
        "boost:without_program_options": False,
        "boost:without_serialization": False,
        "boost:without_exception": False,
        "boost:without_container": False,
        "boost:without_chrono": False,
        "boost:without_date_time": False,
        "boost:without_atomic": False,
        "boost:without_math": True,
        "boost:without_wave": True,
        "boost:without_contract": True,
        "boost:without_graph": True,
        "boost:without_iostreams": True,
        "boost:without_locale": True,
        "boost:without_log": True,
        "boost:without_random": True,
        "boost:without_regex": True,
        "boost:without_mpi": True,
        "boost:without_coroutine": True,
        "boost:without_fiber": True,
        "boost:without_context": True,
        "boost:without_timer": True,
        "boost:without_graph_parallel": True,
        "boost:without_python": True,
        "boost:without_type_erasure": True
    }
