# Clustering plugin ![Build Status](https://github.com/ManiVaultStudio/ClusteringPlugin/actions/workflows/build.yml/badge.svg?branch=main)

Clustering plugin for the [ManiVault](https://github.com/ManiVaultStudio/core) visual analytics framework.

```bash
git clone git@github.com:ManiVaultStudio/ClusteringPlugin.git
```

Uses cluster algorithm implementations from [mlpack](https://github.com/mlpack/mlpack) [3-clause BSD].

## Cluster algorithms
- [k-means](https://en.wikipedia.org/wiki/K-means_clustering): [mlpack/tutorials/kmeans.md](https://github.com/mlpack/mlpack/blob/master/doc/tutorials/kmeans.md)
- [Mean-Shift](https://en.wikipedia.org/wiki/Mean_shift): [mlpack/methods/mean_shift.md](https://github.com/mlpack/mlpack/blob/master/doc/user/methods/mean_shift.md)
    - A larger `radius multiplier` value will generally result in fewer clusters (e.g. a coarser clustering); smaller radius values will generally result in more clusters.
- [DBSCAN](https://en.wikipedia.org/wiki/DBSCAN): [mlpack/bindings/cli.md#mlpack_dbscan](https://github.com/mlpack/mlpack/blob/master/doc/user/bindings/cli.md#mlpack_dbscan)
    - `Epsilon`: Radius of each range search 
    - `Min size`: Minimum number of points for a cluster 
