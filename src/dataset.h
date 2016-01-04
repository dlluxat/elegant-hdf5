#ifndef DATASET_H
#define DATASET_H

#include "object.h"
#include "utils.h"
#include "logging.h"

#include <armadillo>
#include <iostream>
#include <typeinfo>

namespace h5cpp {

template<class T>
struct is_vec {
    static constexpr bool value = false;
};

template<>
struct is_vec<arma::vec> {
    static constexpr bool value = true;
};

template<class T>
struct is_vec<arma::Col<T>> {
    static constexpr bool value = true;
};

template<class T>
struct is_vec<arma::Row<T>> {
    static constexpr bool value = true;
};

template<class T>
struct is_mat {
    static constexpr bool value = false;
};

template<class T>
struct is_mat<arma::Mat<T>> {
    static constexpr bool value = true;
};

template<class T>
struct is_cube {
    static constexpr bool value = false;
};

template<class T>
struct is_cube<arma::Cube<T>> {
    static constexpr bool value = true;
};

class Dataset : public Object
{
public:
    Dataset();
    Dataset(hid_t id, hid_t parentID, std::string name);

    Dataset(const Object &other);
    Dataset(const Dataset &other);
    //    Dataset(Dataset &&other);

    ~Dataset();

    Dataset& operator=(const Object &other);
    Dataset& operator=(const Dataset &other);
    //    Dataset& operator=(Object &&other);
    //    Dataset& operator=(Dataset &&other);

    template<typename T>
    bool matchingExtents(const arma::Col<T> &v, hsize_t *extents);

    template<typename T>
    bool matchingExtents(const arma::Row<T> &v, hsize_t *extents);

    template<typename T>
    bool matchingExtents(const arma::Mat<T> &v, hsize_t *extents);

    template<typename T>
    bool matchingExtents(const arma::Cube<T> &v, hsize_t *extents);

    template<typename T>
    static void extentsFromType(const arma::Col<T> &v, hsize_t *extents);

    template<typename T>
    static void extentsFromType(const arma::Row<T> &v, hsize_t *extents);

    template<typename T>
    static void extentsFromType(const arma::Mat<T> &v, hsize_t *extents);

    template<typename T>
    static void extentsFromType(const arma::Cube<T> &v, hsize_t *extents);

    template<typename T>
    Dataset& operator=(const T &data);

    // TODO Support other arma temporaries, like eOp
    template<typename T, typename U>
    Dataset& operator=(const arma::Gen<T, U> &data);

    template<typename T>
    static Dataset create(hid_t parentID, const std::string &name, const T &data);

    // TODO: Support other types of rvalues, such as eOP
    template<typename T, typename U>
    static Dataset create(hid_t parentID, const std::string &name, const arma::Gen<T, U> &data);

    template<typename T>
    operator arma::Row<T>() const;

    template<typename T>
    operator arma::Col<T>() const;

    template<typename T>
    operator arma::Mat<T>() const;

    template<typename T>
    operator arma::Cube<T>() const;
private:
    //    void constructFromOther(const Object &other);
    //    void close();
};

template<typename T>
inline void Object::operator=(const T& matrix)
{
    DLOG(INFO) << "Assignment operator of T";
    DLOG(INFO) << "Is valid: " << isValid();
    if(isValid()) {
        Dataset dataset = *this;
        dataset = matrix;
    } else if(m_parentID > 0) {
        Dataset::create(m_parentID, m_name, matrix);
    }
}

template<typename T>
bool Dataset::matchingExtents(const arma::Col<T> &v, hsize_t *extents) {
    if(v.n_rows == extents[0]) {
        return true;
    }
    return false;
}

template<typename T>
bool Dataset::matchingExtents(const arma::Row<T> &v, hsize_t *extents) {
    if(v.n_cols == extents[0]) {
        return true;
    }
    return false;
}

template<typename T>
bool Dataset::matchingExtents(const arma::Mat<T> &v, hsize_t *extents) {
    if(v.n_rows == extents[0] && v.n_cols == extents[1]) {
        return true;
    }
    return false;
}

template<typename T>
bool Dataset::matchingExtents(const arma::Cube<T> &v, hsize_t *extents) {
    if(v.n_rows == extents[0] && v.n_cols == extents[1]  && v.n_slices == extents[2]) {
        return true;
    }
    return false;
}

template<typename T>
void Dataset::extentsFromType(const arma::Col<T> &v, hsize_t *extents) {
    extents[0] = v.n_rows;
}

template<typename T>
void Dataset::extentsFromType(const arma::Row<T> &v, hsize_t *extents) {
    extents[0] = v.n_cols;
}

template<typename T>
void Dataset::extentsFromType(const arma::Mat<T> &v, hsize_t *extents) {
    extents[0] = v.n_rows;
    extents[1] = v.n_cols;
}

template<typename T>
void Dataset::extentsFromType(const arma::Cube<T> &v, hsize_t *extents) {
    extents[0] = v.n_rows;
    extents[1] = v.n_cols;
    extents[2] = v.n_slices;
}

template<typename T>
Dataset& Dataset::operator=(const T &data)
{
    DLOG(INFO) << "Dataset assignment operator of T type: " << typeid(T).name();
    DLOG(INFO) << "Parent, name, id: " << m_parentID << " " << m_name << " " << m_id;
    if(m_id == 0 && m_parentID > 0) {
        *this = Dataset::create(m_parentID, m_name, data);
    } else {
        int targetDimensions = 2;
        if(is_vec<T>::value) {
            targetDimensions = 1;
        } else if(is_mat<T>::value) {
            targetDimensions = 2;
        } else if(is_cube<T>::value) {
            targetDimensions = 3;
        } else {
            DLOG(INFO) << "ERROR: Could not determine dimensions of object.";
            return *this;
        }
        hid_t dataspace = H5Dget_space(m_id);
        int currentDimensions = H5Sget_simple_extent_ndims(dataspace);

        hsize_t extents[3];
        H5Sget_simple_extent_dims(dataspace, extents, NULL);

        bool shouldOverwrite = false;
        if(currentDimensions != targetDimensions || !matchingExtents(data, extents)) {
            shouldOverwrite = true;
        }

        if(shouldOverwrite) {
            //#ifdef H5CPP_VERBOSE
            FLAGS_stderrthreshold = 1;
            DLOG(WARNING) << "Writing over dataset of shape (" << extents[0] << ", " << extents[1] << ") "
                          << "with matrix of shape (" << data.n_rows << ", " << data.n_cols << "). "
                          << "Limitations in HDF5 standard makes it impossible to free space taken "
                          << "up by the old dataset." << std::endl;
            //#endif
            H5Sclose(dataspace);
            close();
            H5Ldelete(m_parentID, m_name.c_str(), H5P_DEFAULT);
            *this = Dataset::create(m_parentID, m_name, data);
        } else {
            DLOG(INFO) << "Writing to old dataset";
            hid_t datatype = TypeHelper<T>::hdfType();
            herr_t errors = H5Dwrite(m_id, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data[0]);
            H5Sclose(dataspace);
            if(errors < 0) {
                DLOG(INFO) << "Error writing to dataset!";
            }
        }
    }
    return *this;
}

template<typename T, typename U>
Dataset& Dataset::operator=(const arma::Gen<T, U> &data) {
    T dataReal = data;
    *this = dataReal;
    return *this;
}

template<typename T>
Dataset Dataset::create(hid_t parentID, const std::string &name, const T &data)
{
    DLOG(INFO) << "Creating dataset on parent " << parentID << " with name " << name;
    int targetDimensions = 0;
    if(arma::is_Col<T>::value || arma::is_Row<T>::value) {
        targetDimensions = 1;
    } else if(arma::is_Mat<T>::value) {
        targetDimensions = 2;
    } else if(arma::is_Cube<T>::value) {
        targetDimensions = 3;
    } else {
        DLOG(INFO) << "ERROR: Could not determine dimensions of object.";
        return Dataset(0, 0, name);
    }
    hsize_t dims[3];
    extentsFromType(data, dims);
    DLOG(INFO) << "Extents: " << dims[0] << " "
               << (targetDimensions > 1 ? dims[1] : 0) << " "
               << (targetDimensions > 2 ? dims[2] : 0);

    hid_t dataspace = H5Screate_simple(targetDimensions, &dims[0], NULL);
    hid_t creationParameters = H5Pcreate(H5P_DATASET_CREATE);
    hid_t datatype = TypeHelper<T>::hdfType();
    hid_t dataset = H5Dcreate(parentID, name.c_str(), datatype, dataspace,
                              H5P_DEFAULT, creationParameters, H5P_DEFAULT);
    if(dataset > 0) {
        herr_t errors = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data[0]);
        if(errors >= 0) {
            H5Sclose(dataspace);
            DLOG(INFO) << "Returning the created dataset " << dataset;
            return Dataset(dataset, parentID, name);
        }
    };
    return Dataset(0, 0, name);
}

template<typename T, typename U>
Dataset Dataset::create(hid_t parentID, const std::string &name, const arma::Gen<T, U> &data) {
    T dataReal = data;
    return create(parentID, name, dataReal);
}

template<typename T>
Dataset::operator arma::Row<T>() const
{
    DLOG(INFO) << "Making a row";
    hid_t dataspace = H5Dget_space(m_id);
    int dimensionCount = H5Sget_simple_extent_ndims(dataspace);
    DLOG(INFO) << "Dimension count: " << dimensionCount;

    if(dimensionCount != 1) {
        std::cerr << "ERROR: Tried to copy dataspace with "
                  << dimensionCount << " dimensions to arma::Row." << std::endl;
        return arma::Row<T>();
    }

    hsize_t extents[dimensionCount];

    H5Sget_simple_extent_dims(dataspace, extents, NULL);
    DLOG(INFO) << "Extents: " << extents[0];

    arma::Row<T> matrix(extents[0]);

    hid_t hdf5Datatype = TypeHelper<T>::hdfType();
    H5Dread(m_id, hdf5Datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &matrix[0]);
    return matrix;
}

template<typename T>
Dataset::operator arma::Col<T>() const
{
    hid_t dataspace = H5Dget_space(m_id);
    int dimensionCount = H5Sget_simple_extent_ndims(dataspace);

    if(dimensionCount != 1) {
        std::cerr << "ERROR: Tried to copy dataspace with "
                  << dimensionCount << " dimensions to arma::Col." << std::endl;
        return arma::Mat<T>();
    }

    hsize_t extents[dimensionCount];
    H5Sget_simple_extent_dims(dataspace, extents, NULL);

    arma::Col<T> matrix(extents[0]);

    hid_t hdf5Datatype = TypeHelper<T>::hdfType();
    H5Dread(m_id, hdf5Datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &matrix[0]);
    return matrix;
}

template<typename T>
Dataset::operator arma::Mat<T>() const
{
    hid_t dataspace = H5Dget_space(m_id);
    int dimensionCount = H5Sget_simple_extent_ndims(dataspace);

    if(dimensionCount != 2) {
        std::cerr << "ERROR: Tried to copy dataspace with "
                  << dimensionCount << " dimensions to arma::mat." << std::endl;
        return arma::Mat<T>();
    }

    hsize_t extents[dimensionCount];
    H5Sget_simple_extent_dims(dataspace, extents, NULL);

    arma::Mat<T> matrix(extents[0], extents[1]);

    hid_t hdf5Datatype = TypeHelper<T>::hdfType();
    H5Dread(m_id, hdf5Datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &matrix[0]);
    return matrix;
}

template<typename T>
Dataset::operator arma::Cube<T>() const
{
    hid_t dataspace = H5Dget_space(m_id);
    int dimensionCount = H5Sget_simple_extent_ndims(dataspace);

    if(dimensionCount != 3) {
        std::cerr << "ERROR: Tried to copy dataspace with "
                  << dimensionCount << " dimensions to arma::Cube." << std::endl;
        return arma::Cube<T>();
    }

    hsize_t extents[dimensionCount];
    H5Sget_simple_extent_dims(dataspace, extents, NULL);

    arma::Cube<T> cube(extents[0], extents[1], extents[2]);

    hid_t hdf5Datatype = TypeHelper<T>::hdfType();
    H5Dread(m_id, hdf5Datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &cube[0]);

    H5Sclose(dataspace);
    return cube;
}

}

#endif // DATASET_H
